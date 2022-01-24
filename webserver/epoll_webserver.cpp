//
// Created by yjs on 2022/1/23.
//
#include "wrap.h"
#include <algorithm>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>


namespace fs = std::filesystem;

using namespace std;


static const int SERVER_PORT = 8000;
static const char SERVER_IP[] = "0.0.0.0";
static const int EpollEvsSize = 1024;
static const short EpollWaitTimeout = -1;
static const short ReadBufferSize = 1500;
extern std::string UrlDecode(const std::string &);

unordered_map<int, string> map_port;

/// show funcs
static void read_client_requests(int *, epoll_event *);
static void send_http_headers(int, const int &, const string &, const string &, const int &);
static int send_file_requests(int, int *, epoll_event *, const string &, const bool &);
static string get_mime_type(const string &);

static vector<string> split(const string &, const string &);


int main() {


    signal(SIGPIPE, SIG_IGN);

    // 获取当前目录的工作路径
    fs::path pwd_path = fs::current_path();
    pwd_path /= "web-http";
    // 切换目录

    chdir(pwd_path.string().c_str());

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
    //    close(epoll_fd);
    //    close(listen_fd);
    return 0;
}


static vector<string> split(const string &str, const string &delim) {//将分割后的子字符串存储在vector中
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
static string get_mime_type(const string &str) {
    string dot;
    size_t pos = str.rfind(".");
    //        res=str.substr(pos);
    //        cout<< res<<endl;
    if (pos == string::npos) {
        return string("text/plain; charset=utf-8");
    } else {
        dot = str.substr(pos);
    }
    if (dot == ".html" || dot == ".htm")
        return string("text/html; charset=utf-8");
    if (dot == ".jpg" || dot == ".jpeg")
        return string("image/jpeg");
    if (dot == ".gif")
        return string("image/gif");
    if (dot == ".png")
        return string("image/png");
    if (dot == ".css")
        return string("text/css");
    if (dot == ".au")
        return string("audio/basic");
    if (dot == ".wav")
        return string("audio/wav");
    if (dot == ".avi")
        return string("video/x-msvideo");
    if (dot == ".mov" || dot == ".qt")
        return string("video/quicktime");
    if (dot == ".mpeg" || dot == ".mpe")
        return string("video/mpeg");
    if (dot == ".vrml" || dot == ".wrl")
        return string("model/vrml");
    if (dot == ".midi" || dot == ".mid")
        return string("audio/midi");
    if (dot == ".mp3")
        return string("audio/mpeg");
    if (dot == ".ogg")
        return string("application/ogg");
    if (dot == ".pac")
        return string("application/x-ns-proxy-autoconfig");
    return string("text/plain; charset=utf-8");
};


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

    while ((ret = Readline(ev->data.fd, TmpBuffer, sizeof(TmpBuffer))) > 0)
        ;

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
    string request_head1(buffer);
    vector<string> res = split(request_head1, " ");
    string method, path, http_version;
    method = res[0];
    path = res[1];
    http_version = res[2];


    // 判断是否为GET 请求
    transform(method.begin(), method.end(), method.begin(), ::tolower);
    if (method == "get") {
        string string_file = UrlDecode(path.substr(1));
        // 判断请求的文件在不在
        //        fs::path dir(string_file);
        if (string_file.empty()) {
            // 127.0.0.1:8000/ 没有请求的文件 默认请求当前目录
            cout << "path [ "
                 << "/"
                 << " ] exists " << endl;
            string_file = "./";
        }
        if (fs::exists(string_file)) {
            cout << "path [ " << string_file << " ] exists " << endl;
            if (fs::is_directory(string_file)) {
                // 是一个目录

                cout << "is a directory " << endl;

                // 发送一个列表

                send_http_headers(ev->data.fd, 200, string("OK"), get_mime_type("*.html"), 0);
                // 发送 dir_header.html
                send_file_requests(ev->data.fd, epoll_fd, ev, "dir_header.html", false);


                vector<string> dirs;
                string string_li;
                if (fs::is_directory(string_file)) {
                    for (fs::directory_entry entry: fs::directory_iterator(string_file)) {
                        if (entry.is_directory()) {
                            dirs.push_back(entry.path().filename().string() + "/");

                        } else {
                            dirs.push_back(entry.path().filename().string());
                        }
                    }
                }
                for (string li: dirs) {

                    string_li = "<li><a href=\"" + li + "\">" + li + "</a></li>";
                    send(ev->data.fd, string_li.c_str(), string_li.size(), 0);
                    cout << string_li << endl;
                    string_li.clear();
                }

                // 发送 dir_tail.html
                send_file_requests(ev->data.fd, epoll_fd, ev, "dir_tail.html", true);


            } else {
                // 是一个文件
                cout << "is a file " << endl;
                // 先发送报头 （状态行 消息头）
                struct stat s;
                stat(string_file.c_str(), &s);
                send_http_headers(ev->data.fd, 200, string("OK"), get_mime_type(string_file), s.st_size);
                // 发送文件
                send_file_requests(ev->data.fd, epoll_fd, ev, string_file, false);
            }


        } else {
            cout << "path [ " << string_file << " ] not exists " << endl;
            // 先发送报头 （状态行 消息头）
            send_http_headers(ev->data.fd, 404, string("NOT FOUND"), get_mime_type("*.html"), 0);
            // 发送 error.html
            send_file_requests(ev->data.fd, epoll_fd, ev, "error.html", false);
        }
    }
}


static void send_http_headers(int cfd, const int &status_code, const string &info, const string &file_type, const int &length) {

    // 发送状态行
    string buffer = "HTTP/1.1 " + to_string(status_code) + " " + info + "\r\n";


    send(cfd, buffer.c_str(), buffer.size(), 0);

    // 发送消息头
    buffer.clear();
    buffer = "Content-Type:" + file_type + "\r\n";
    send(cfd, buffer.c_str(), buffer.size(), 0);

    if (length > 0) {
        // 发送长度
        buffer.clear();
        buffer = "Content-Length:" + to_string(length) + "\r\n";
        send(cfd, buffer.c_str(), buffer.size(), 0);
    }
    // 发送空行
    send(cfd, "\r\n", 2, 0);
}


static int send_file_requests(int cfd, int *epoll_fd, epoll_event *ev, const string &file_path, const bool &is_closed) {

    fstream files;
    files.open(file_path, ios_base::in);
    if (!files) {

        cerr << "unable to open file" << endl;
        return 1;
    }
    string file_count;
    int ch;
    while ((ch = files.get()) != EOF) {

        file_count += ch;
    }
    send(cfd, file_count.c_str(), file_count.size(), 0);


    files.close();
    // 关闭cfd 下树

    if (is_closed) {

        close(cfd);
        //对端关闭了连接，从epoll_fd上移除 client_fd
        epoll_ctl(*epoll_fd, EPOLL_CTL_DEL, ev->data.fd, ev);
    }


    return 0;
}
