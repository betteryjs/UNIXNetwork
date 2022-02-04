//
// Created by yjs on 2022/1/8.
//

#include "wrap.h"
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/epoll.h>
#include <unordered_map>


using namespace std;

const short Port = 8000;
const char *ServerIp = "127.0.0.1";
const short EvsSize = 1024;
const short BufferSize = 1500;
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

        if (n_ready < 0) {

            perror("epoll_wait");
            break;

        } else if (n_ready == 0) {
            continue;
        } else {
            // 监听到了数据 有文件描述符变化
            for (int i = 0; i < n_ready; ++i) {

                // 判断lfd变化 并且是读事件变化
                if (evs[i].data.fd == listen_fd && (evs[i].events & EPOLLIN)) {
                    // accept 提取cfd


                    sockaddr_in client_address;
                    socklen_t client_len = sizeof(client_address);
                    int client_fd = Accept(evs[i].data.fd, (sockaddr *) &client_address, &client_len);


                    // print ip and port
                    char ip[INET_ADDRSTRLEN] = "";
                    //                        char *ip;
                    inet_ntop(AF_INET, &client_address.sin_addr.s_addr, ip, 16);
                    cout << "new client connect . . . and ip is " << ip << " port is : " << ntohs(client_address.sin_port) << endl;

                    // save client ip:port
                    map[client_fd] = string(ip) + ":" +
                                     to_string(ntohs(client_address.sin_port));
                    // 将 client_fd  上树
                    ev.data.fd = client_fd;
                    ev.events = EPOLLIN;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);

                } else if (evs[i].events &
                           EPOLLIN) { // cfd变化 并且是读事件变化
                  ssize_t n;
                  char buffer[BufferSize];
                  memset(buffer, 0, sizeof(buffer));

                  if ((n = Read(evs[i].data.fd, buffer, BufferSize)) < 0) {

                    perror("read error");
                    epoll_ctl(epfd, EPOLL_CTL_DEL, evs[i].data.fd, &evs[i]);

                  } else if (n == 0) {

                    cout << "client [ " << map[evs[i].data.fd] << " ] aborted connection" << endl;
                        epoll_ctl(epfd, EPOLL_CTL_DEL, evs[i].data.fd, &evs[i]);

                    } else {

                        cout << "client " << map[evs[i].data.fd] << " data : " << buffer << endl;
                        string str(buffer);
                        str = "server data : " + str;
                        Write(evs[i].data.fd, str.c_str(), str.size());
                    }
                }
            }
        }
    }

    return 0;
}
