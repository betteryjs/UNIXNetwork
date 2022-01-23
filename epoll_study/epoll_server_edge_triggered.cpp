//
// Created by yjs on 2022/1/8.
//

#include <iostream>
#include <arpa/inet.h>
#include "wrap.h"
#include <sys/epoll.h>
#include <string>
#include <netinet/in.h>
#include <unordered_map>
#include <fcntl.h>

using namespace std;


const int ServePort = 8001;
const char *ServerIp = "127.0.0.1";
const int EpollEvsSize = 1024;
const short ReadBufferSize = 4;
const short EpollWaitTimeout = -1;


int main() {

    // 创建套结字
    int listen_fd;
    listen_fd = tcp4bind(ServePort, ServerIp);
    // 监听套结字
    Listen(listen_fd, 128);
    // 创建红黑树
    int epoll_fd = epoll_create(1);

    if (epoll_fd == -1) {
        cout << "create epollfd error." << endl;
        close(listen_fd);
        return -1;
    }

    // 将listen_fd上树
    //将监听 sokcet 绑定到 epoll_fd 上去

    epoll_event ev, evs[EpollEvsSize];
    ev.data.fd = listen_fd;
    ev.events = EPOLLIN;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
        cerr << "epoll_ctl error" << endl;
        close(listen_fd);
        return -1;

    };


    unordered_map<int, string> map;

    // while监听
    while (true) {
        int n_ready = epoll_wait(epoll_fd, evs, EpollEvsSize, EpollWaitTimeout);

        if (n_ready < 0) {
            //被信号中断
            if (errno == EINTR)
                continue;
            perror("epoll_wait");
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
                    char *ip = (char *) malloc(INET_ADDRSTRLEN);

                    inet_ntop(AF_INET, &client_address.sin_addr.s_addr, ip, 16);
                    cout << "new client connect . . . and ip is " << ip << " port is : " <<
                         ntohs(client_address.sin_port) << endl;
                    map[client_fd] = string(ip) + ":" + to_string(ntohs(client_address.sin_port));
                    cout << "epoll wait __ip_______________________________" << endl;
                    free(ip);
                    ip = nullptr;
//                    memset(ip,1, sizeof(ip));



                    // 设置cfd为非阻塞
                    int flag = fcntl(client_fd, F_GETFL);
                    flag |= O_NONBLOCK;
                    fcntl(client_fd, F_SETFL, flag);


                    // 将 client_fd  上树
                    ev.data.fd = client_fd;
                    ev.events = EPOLLIN | EPOLLET;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);

                }
                    // cfd变化 并且是读事件变化
                else if (evs[i].events & EPOLLIN) {
                    while (true) {
                        ssize_t n;
                        char buffer[ReadBufferSize];
                        // 如果读一个缓冲区 缓冲区没有数据 如果是带阻塞 就阻塞等待
                        // 如果是非阻塞 返回值是-1 并将 erron 设置为 EINTR
                        n = read(evs[i].data.fd, buffer, ReadBufferSize);
                        if (n < 0) {
                            // 如果缓冲区读干净了 break 继续监听

                            if (errno == EAGAIN) {
                                break;
                            } else {
                                //出错
                                perror("read error");
                                close(evs[i].data.fd);
                                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, evs[i].data.fd, &evs[i]);
                                break;
                            }

                        } else if (n == 0) {

                            //对端关闭了连接，从 epoll_fd 上移除 client_fd
                            cout << "client [ " << map[evs[i].data.fd] << " ] aborted connection" << endl;
                            close(evs[i].data.fd);
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, evs[i].data.fd, &evs[i]);
                            break;
                        } else {
                            cout << "epoll wait clint__________________________________" << endl;

                            cout << "client " << map[evs[i].data.fd] << " data : " << buffer << endl;
                            string str(buffer);
                            str = "server data : " + str;
                            Write(evs[i].data.fd, str.c_str(), str.size());
                            // 没有使用buffer数据 所以需要memset()
                            memset(buffer, 0, sizeof(buffer));
                        }
                    }
                }
            }
        }
    }

    return 0;

}
