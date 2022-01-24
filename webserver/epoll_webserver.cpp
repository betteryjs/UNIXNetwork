//
// Created by yjs on 2022/1/23.
//
#include "wrap.h"
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>
#include <unordered_map>
#include <unordered_set>
#include <regex>


using namespace std;


static const int SERVER_PORT = 8000;
static const char SERVER_IP[] = "0.0.0.0";
static const int EpollEvsSize = 1024;
static const short EpollWaitTimeout = -1;
static const short ReadBufferSize = 1500;

static void read_client_requests(int *, epoll_event *);
vector<string> split(const string &, const string &);

unordered_map<int, string> map_port;


int main() {

    // 创建套接字
    int listen_fd = tcp4bind(SERVER_PORT, SERVER_IP);
    // 监听
    Listen(listen_fd, 128);
    //创建树

    int epoll_fd = epoll_create(1);

    if (epoll_fd == -1) {
        cout << "create epollfd error." << endl;
        close(listen_fd);
        return -1;
    }

    // 将lfd 上树


    epoll_event ev, evs[EpollEvsSize];
    ev.data.fd = listen_fd;
    ev.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
        cerr << "epoll_ctl error" << endl;
        close(listen_fd);
        return 1;
    };
    //循环监听

    while (true) {

        int n_ready = epoll_wait(epoll_fd, evs, EpollEvsSize, EpollWaitTimeout);

        if (n_ready < 0) {
            //被信号中断
            if (errno == EINTR)
                continue;
            cerr << "epoll_wait error . . ." << endl;
            break;
        } else if (n_ready == 0) {
            //超时,继续
            continue;
        } else {
            // 监听到了数据 有文件描述符变化
            for (ssize_t i = 0; i < n_ready; ++i) {
                // 判断lfd变化 并且是读事件变化
                if (evs[i].data.fd == listen_fd && (evs[i].events & EPOLLIN)) {
                    // accept 提取cfd

                    sockaddr_in client_address;
                    socklen_t client_len = sizeof(client_address);
                    int client_fd = Accept(evs[i].data.fd, (sockaddr *) &client_address, &client_len);

                    // save client ip:port
                    //                    char ip[INET_ADDRSTRLEN]={0};
                    char ip[INET_ADDRSTRLEN] = "";
                    inet_ntop(AF_INET, &client_address.sin_addr.s_addr, ip, 16);
                    cout << "new client connect . . . and ip is " << ip << " port is : " << ntohs(client_address.sin_port) << endl;
                    map_port[client_fd] = string(ip) + ":" + to_string(ntohs(client_address.sin_port));


                    // 设置cfd为非阻塞
                    int flag = fcntl(client_fd, F_GETFL);
                    flag |= O_NONBLOCK;
                    fcntl(client_fd, F_SETFL, flag);


                    // 将 client_fd  上树
                    ev.data.fd = client_fd;
                    ev.events = EPOLLIN;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);


                } else if (evs[i].events & EPOLLIN) {

                    // cfd 变化
                    read_client_requests(&epoll_fd, &evs[i]);
                }
            }
        }
    }


    // close


    return 0;
}


vector<string> split(const string &str, const string &delim) {//将分割后的子字符串存储在vector中
    vector<string> res;
    if (str.empty()) return res;
    string strs = str + delim;//*****扩展字符串以方便检索最后一个分隔出的字符串
    size_t pos;
    size_t size = strs.size();

    for (int i = 0; i < size; ++i) {
        pos = strs.find(delim, i);             //pos为分隔符第一次出现的位置，从i到pos之前的字符串是分隔出来的字符串
        if (pos < size) {                      //如果查找到，如果没有查找到分隔符，pos为string::npos
            string s = strs.substr(i, pos - i);//*****从i开始长度为pos-i的子字符串
            res.push_back(s);                  //两个连续空格之间切割出的字符串为空字符串，这里没有判断s是否为空，所以最后的结果中有空字符的输出，
            i = pos + delim.size() - 1;
        }
    }
    return res;
}
static void read_client_requests(int *epoll_fd, epoll_event *ev) {

    // 读取请求 (先读取一行，再把其它行读取扔掉)
    // ssize_t Readline(int fd, void *vptr, size_t maxlen) ;
    char buffer[ReadBufferSize] = "";
    char TmpBuffer[ReadBufferSize] = "";

    memset(buffer, 0, sizeof(buffer));
    ssize_t n = Readline(ev->data.fd, buffer, sizeof(buffer));

    if (n < 0) {
        cerr << "read  error . . ." << endl;
        epoll_ctl(*epoll_fd, EPOLL_CTL_DEL, ev->data.fd, ev);
        close(ev->data.fd);
        return;
    } else if (n == 0) {
        cout << "client [ " << map_port[ev->data.fd] << " ] aborted connection" << endl;
        epoll_ctl(*epoll_fd, EPOLL_CTL_DEL, ev->data.fd, ev);
        close(ev->data.fd);
        return;
    }

    cout << "[" << buffer << "]" << endl;
    ssize_t ret = 0;

    while ((ret = Readline(ev->data.fd, TmpBuffer, sizeof(TmpBuffer))) > 0) {
        ;
    }
    // [GET / HTTP/1.1]
    // 解析请求

    // Regex
    //    std::string method, path, http_version;
    //    string pattern{"(get|post)\\s+(.*+)\\s+([[:alpha:]]+.*+)\n?]"};
    //    // [GET / HTTP/1.1]
    //    regex reg(pattern,regex::icase);
    //    string request_head1(buffer);
    //    for (sregex_iterator iterator(request_head1.begin(),request_head1.end(),reg),end_it;iterator!=end_it;++iterator) {
    //        method=iterator->str(1);
    //        path=iterator->str(2);
    //        http_version=iterator->str(3);
    //    }


    // find
    cout << "find ..... ......." << endl;
    string request_head1(buffer);
    vector<string> res = split(request_head1, " ");
    string method, path, http_version;
    method = res[0];
    path = res[1];
    http_version = res[2];


    // 判断是否为GET 请求
    // 得到web请求的路径
    // 判读文件是否存在 如果存在(普通文件 目录)

    // 不存在发送 error.html
}
