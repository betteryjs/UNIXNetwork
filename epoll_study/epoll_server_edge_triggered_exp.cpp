/** 
  * 验证epoll的LT与ET模式的区别, epoll_server.cpp
  * zhangyl 2019.04.01
  */
#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>


using namespace std;

int main() {
    //创建一个监听socket
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        cout << "create listen socket error" << endl;
        return -1;
    }

    //设置重用ip地址和端口号
    int on = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, (char *) &on, sizeof(on));


    //将监听socker设置为非阻塞的
    int oldSocketFlag = fcntl(listenfd, F_GETFL, 0);
    int newSocketFlag = oldSocketFlag | O_NONBLOCK;
    if (fcntl(listenfd, F_SETFL, newSocketFlag) == -1) {
        close(listenfd);
        cout << "set listenfd to nonblock error" << endl;
        return -1;
    }

    //初始化服务器地址
    struct sockaddr_in bindaddr;
    bindaddr.sin_family = AF_INET;
    bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bindaddr.sin_port = htons(8000);

    if (bind(listenfd, (struct sockaddr *) &bindaddr, sizeof(bindaddr)) == -1) {
        cout << "bind listen socker error." << endl;
        close(listenfd);
        return -1;
    }

    //启动监听
    if (listen(listenfd, SOMAXCONN) == -1) {
        cout << "listen error." << endl;
        close(listenfd);
        return -1;
    }


    //创建epollfd
    int epollfd = epoll_create(1);
    if (epollfd == -1) {
        cout << "create epollfd error." << endl;
        close(listenfd);
        return -1;
    }

    epoll_event listen_fd_event;
    listen_fd_event.data.fd = listenfd;
    listen_fd_event.events = EPOLLIN;
    //取消注释掉这一行，则使用ET模式
    listen_fd_event.events |= EPOLLET;

    //将监听sokcet绑定到epollfd上去
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &listen_fd_event) == -1) {
        cout << "epoll_ctl error" << endl;
        close(listenfd);
        return -1;
    }

    int n;
    unordered_map<int, string> map;

    while (true) {
        epoll_event epoll_events[1024];
        n = epoll_wait(epollfd, epoll_events, 1024, 1000);
        if (n < 0) {
            //被信号中断
            if (errno == EINTR)
                continue;

            //出错,退出
            break;
        } else if (n == 0) {
            //超时,继续
            continue;
        }
        for (size_t i = 0; i < n; ++i) {
            //事件可读
            if (epoll_events[i].events & EPOLLIN) {
                if (epoll_events[i].data.fd == listenfd) {
                    //侦听socket,接受新连接
                    struct sockaddr_in clientaddr;
                    socklen_t clientaddrlen = sizeof(clientaddr);
                    int clientfd = accept(listenfd, (struct sockaddr *) &clientaddr, &clientaddrlen);


                    if (clientfd != -1) {
                        int oldSocketFlag = fcntl(clientfd, F_GETFL, 0);
                        int newSocketFlag = oldSocketFlag | O_NONBLOCK;
                        if (fcntl(clientfd, F_SETFD, newSocketFlag) == -1) {
                            close(clientfd);
                            cout << "set clientfd to nonblocking error." << endl;
                        } else {
                            epoll_event client_fd_event;
                            client_fd_event.data.fd = clientfd;
                            client_fd_event.events = EPOLLIN;
                            //取消注释这一行，则使用ET模式
                            client_fd_event.events |= EPOLLET;
                            if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &client_fd_event) != -1) {
                                //                                cout << "new client accepted,clientfd: " << clientfd << endl;
                                char ip[INET_ADDRSTRLEN] = "";
                                inet_ntop(AF_INET, &clientaddr.sin_addr.s_addr, ip, 16);
                                cout << "new client connect . . . and ip is " << ip << " port is : " << ntohs(clientaddr.sin_port) << endl;
                                // save client ip:port
                                map[clientfd] = string(ip) + ":" + to_string(ntohs(clientaddr.sin_port));
                                //                                free(ip);
                                memset(ip, 0, sizeof(ip));


                            } else {
                                cout << "add client fd to epollfd error" << endl;
                                close(clientfd);
                            }
                        }
                    }
                } else {
                    while (true) {
                        //                        cout << "client fd: " << epoll_events[i].data.fd << " recv data." << endl;
                        //普通clientfd
                        cout << "client [ " << map[epoll_events[i].data.fd] << " ] aborted connection" << endl;

                        char ch[4];
                        //每次只收一个字节
                        int m = read(epoll_events[i].data.fd, ch, 4);
                        if (m == 0) {
                            //对端关闭了连接，从epollfd上移除clientfd
                            if (epoll_ctl(epollfd, EPOLL_CTL_DEL, epoll_events[i].data.fd, NULL) != -1) {
                                cout << "client disconnected,clientfd:" << epoll_events[i].data.fd << endl;
                            }
                            close(epoll_events[i].data.fd);
                            break;
                        } else if (m < 0) {
                            //出错
                            if (errno == EAGAIN) {

                                break;
                            }
                            if (errno != EWOULDBLOCK && errno != EINTR) {
                                if (epoll_ctl(epollfd, EPOLL_CTL_DEL, epoll_events[i].data.fd, NULL) != -1) {
                                    cout << "client disconnected,clientfd:" << epoll_events[i].data.fd << endl;
                                }
                                close(epoll_events[i].data.fd);
                            }
                            break;
                        } else {
                            //正常收到数据
                            cout << "client " << map[epoll_events[i].data.fd] << " data :" << ch << endl;
                            memset(ch, 0, sizeof(ch));
                        }
                    }
                }
            } else if (epoll_events[i].events & EPOLLERR) {
                // TODO 暂不处理
            }
        }
    }

    close(listenfd);
    return 0;
}
