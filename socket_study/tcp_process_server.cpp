//
// Created by yjs on 2022/1/5.
//


#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <bits/sigaction.h>
#include <csignal>
#include <wait.h>
#include "wrap.h"

using namespace std;

// 多进程实现并发服务器
void free_process(int sig){
    pid_t pid;
    while (true) {
        pid = waitpid(-1, nullptr, WNOHANG);
        if(pid<=0){
            // pid<0子进程全部退出
            // pid=0  没有进程退出
            break;
        }else {
            cout<< "回收了子进程 pid is :"<< pid<<endl;
        }
    }
}


int main() {
    // 创建套接字 绑定

    int lfd = tcp4bind(8080, "192.168.2.249");
    // bind 绑定
    //  listen 监听
    Listen(lfd, 128);
    // 提取 where(true){
    //   提取连接
    //   fork创建子进程
    //   子进程中关闭lfd(监听套接字) 服务客户端
    //   父进程关闭cfd(连接套接字) 回收子进程的资源
    // }
    struct sockaddr_in client_address;
    socklen_t len = sizeof(client_address);
    while (true) {
        char ip[16] = "";
        // 提取连接
        int cfd = Accept(lfd, (struct sockaddr *) &client_address, &len);
        if (cfd < 0) {
            perror("accept error");
            return 1;
        }
        inet_ntop(AF_INET, &client_address.sin_addr.s_addr, ip, 16);
        cout << "new client connect . . . and ip is " << ip << " port is : " <<
             ntohs(client_address.sin_port) << endl;

        // fork 创建子进程
        pid_t pid;
        if ((pid = fork()) < 0) {
            perror("fork error");
            exit(1);
        } else if (pid == 0) {
            // child
            close(lfd);
            while (true) {
                char buff[2048];
                memset(buff,0, sizeof(buff));
                string str;
                str.clear();
                ssize_t n = read(cfd, buff, sizeof(buff));
                if (n < 0) {
                    perror("read error");
                    close(cfd);
                    exit(0);
                } else if (n == 0) {
                    // 对方关闭了连接
                    cout << "client close . . ." << endl;
                    close(cfd);
                    exit(0);
                } else {
                    // read date from client
                    cout << "client data : " << buff << endl;
                    string  str(buff);
                    str = "server data : " + str;
                    write(cfd, str.c_str(), str.size());






                }
            }
        } else {
            // father
            close(cfd);
            // 回收资源

            // 注册信号回调
            struct sigaction act;
            act.sa_flags=0;
            act.sa_handler=free_process;
            sigemptyset(&act.sa_mask);
            sigaction(SIGCHLD,&act, nullptr);

        }
    }


    // close all connect
    return 0;

}

