//
// Created by yjs on 2022/1/8.
//

#include "wrap.h"
#include <arpa/inet.h>
#include <cctype>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/epoll.h>

using namespace std;

int main() {

    int fd[2];
    pid_t pid;
    pipe(fd);
    // 创建子进程
    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        // child
        close(fd[0]);
        char ch = 'a';
        char buff[5];
        while (true) {
            sleep(1);
            memset(buff, ch++, sizeof(buff));
            write(fd[1], buff, 5);
        }
    } else {
        close(fd[1]);
        // 创建树
        int epfd = epoll_create(1);
        epoll_event ev, evs[1];
        ev.data.fd = fd[0];
        ev.events = EPOLLIN;
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd[0], &ev);// 上树
        // 监听
        while (true) {

            int n = epoll_wait(epfd, evs, 1, -1);
            if (n == 1) {
                char buff[128] = "";
                ssize_t ret = read(fd[0], buff, sizeof(buff));

                if (ret <= 0) {
                    close(fd[0]);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd[0], &ev);//  下树
                    break;
                } else {
                    cout << buff << endl;
                }
            }
        }
    }

    return 0;
}
