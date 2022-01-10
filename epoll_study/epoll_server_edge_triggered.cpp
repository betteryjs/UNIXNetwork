//
// Created by yjs on 2022/1/8.
//

//#include <iostream>
//#include <arpa/inet.h>
#include "wrap.h"
#include <sys/epoll.h>
//#include <string>
//#include <netinet/in.h>
//#include <unordered_map>
#include <bits/stdc++.h>
#include <fcntl.h>

using namespace std;


const short Port = 8000;
const char *ServerIp = "192.168.2.249";
const short EvsSize = 1024;
const short BufferSize = 4;
const short EpollWaitTimeout = -1;


int main() {

    // 创建套结字
    int listen_fd;
    listen_fd = tcp4bind(Port, ServerIp);
    Listen(listen_fd, 128);



    // 创建红黑树

    int epfd = epoll_create(1);

    // 将listen_fd上树

    epoll_event ev, evs[EvsSize];
    ev.data.fd = listen_fd;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);
    unordered_map<int, string> map;
    // while监听
    while (true) {

        int n_ready = epoll_wait(epfd, evs, EvsSize, EpollWaitTimeout);
        cout << "epoll wait __________________________________________" << endl;

        if (n_ready < 0) {

            perror("epoll_wait");
            break;

        } else if (n_ready == 0) {
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

                    char ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &client_address.sin_addr.s_addr, ip, 16);
                    cout << "new client connect . . . and ip is " << ip << " port is : " <<
                                                  ntohs(client_address.sin_port) << endl;
                    map[client_fd] = string(ip) + ":" + to_string(ntohs(client_address.sin_port));
                    memset(ip,0, sizeof(ip));



                    // 设置cfd为非阻塞
                    int flag = fcntl(client_fd, F_GETFL);
                    flag |= O_NONBLOCK;
                    fcntl(client_fd, F_SETFL, flag);



                    // 将 client_fd  上树
                    ev.data.fd = client_fd;
                    ev.events = EPOLLIN | EPOLLET;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);


                } else if (evs[i].events & EPOLLIN) {  // cfd变化 并且是读事件变化
                    while (true) {


                        ssize_t n;
                        char buffer[BufferSize];
                        // 如果读一个缓冲区 缓冲区没有数据 如果是带阻塞 就阻塞等待
                        // 如果是非阻塞 返回值是-1 并将 erron 设置为 EINTR
                        n = read(evs[i].data.fd, buffer, BufferSize);
                        if (n < 0) {

                            // 如果缓冲区读干净了 break 继续监听
                            if (errno == EAGAIN) {

                                break;

                            }

                            perror("read error");
                            close(evs[i].data.fd);
                            epoll_ctl(epfd, EPOLL_CTL_DEL, evs[i].data.fd, &evs[i]);

                            break;

                        } else if (n == 0) {

                            cout << "client [ " << map[evs[i].data.fd] << " ] aborted connection" << endl;
                            close(evs[i].data.fd);
                            epoll_ctl(epfd, EPOLL_CTL_DEL, evs[i].data.fd, &evs[i]);
                            break;
                        } else {

                            cout << "client " << map[evs[i].data.fd ]<< " data : " << buffer << endl;
                            string str(buffer);
                            str = "server data : " + str;
                            Write(evs[i].data.fd, str.c_str(), str.size());
                            memset(buffer, 0, sizeof(buffer));


                        }
                    }

                }

            }


        }


    }

    return 0;

}
