//
// Created by yjs on 2022/1/7.
//

#include "wrap.h"
#include <arpa/inet.h>
#include <cctype>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <unordered_map>

using namespace std;

const int BufferSize = 1500;
const unsigned short Server_Port = 8000;
const int Open_Max = 1024;
const char SERVER_IP[]="0.0.0.0";



int main(int argc, char **argv) {


    int listen_fd, conn_fd, sock_fd;
    int n_ready;// 接收poll返回值记录满足监听事件的fd个数
    ssize_t n;
    char buff[BufferSize];
    socklen_t client_len;
    struct sockaddr_in client_address;
    struct pollfd client[Open_Max];
    unordered_map<int, string> map;

    listen_fd = tcp4bind(Server_Port, SERVER_IP);


    Listen(listen_fd, 128);


    client[0].fd = listen_fd; // 要监听的第一个文件描述符 存入client[0]
    client[0].events = POLLIN;// 监听读事件


    for (int i = 1; i < Open_Max; ++i) {
        client[i].fd = -1;// 用-1初始化client[]里剩下元素
    }

    int max_i = 0;// client[] 数组有效元素的最大值的下标
    while (true) {

        memset(buff, 0, sizeof(buff));

        n_ready = poll(client, max_i + 1, -1);// 阻塞监听是否有客户端链接请求

        if (client[0].revents & POLLIN) {// 有读事件
            // accept form listen fd
            client_len = sizeof(client_address);
            conn_fd = Accept(listen_fd, (struct sockaddr *) &client_address, &client_len);

            // print ip and port
            char ip[INET_ADDRSTRLEN] = "";
            inet_ntop(AF_INET, &client_address.sin_addr.s_addr, ip, 16);
            cout << "new client connect . . . and ip is " << ip << " port is : " << ntohs(client_address.sin_port) << endl;

            // 将新提取的cfd加入监听的队列 遍历找到第一个空闲的位置
            int index;
            for (index = 1; index < Open_Max; ++index) {
                if (client[index].fd < 0) {
                    client[index].fd = conn_fd;// 找到client[]中的空闲位置 存放accept返回的的conn_fd
                    map[index] = string(ip) + ":" + to_string(ntohs(client_address.sin_port));
                    break;
                }
            }

            // 如果满了抛出错误
            if (index == Open_Max) {
                perror("to much clients . . .");
                exit(1);
            }
            // 设置 刚返回的 conn_fd 监控读事件
            client[index].events = POLLIN;
            // 更新 max_i
            if (index > max_i) {
                max_i = index;
            }
            // 没有变化 进入下一个循环
            if (--n_ready <= 0) {
                continue;
            }
        }


        for (int i = 1; i <= max_i; ++i) {
            if ((sock_fd = client[i].fd) < 0) {
                // 找到第一个大于0的
                continue;
            }

            if (client[i].revents & POLLIN) {
                if ((n = Read(sock_fd, buff, BufferSize)) < 0) {
                    if (errno == ECONNREFUSED) {
                        //  收到RST标志
                        cout << "client[" << i << "] aborted connection" << endl;
                        Close(sock_fd);
                        client[i].fd = -1;
                    } else {
                        perror("read error");
                        exit(1);
                    }
                } else if (n == 0) {
                    cout << "client[" << i << "] aborted connection" << endl;
                    Close(sock_fd);
                    client[i].fd = -1;
                } else {
                    for (int j = 0; j < n; ++j) {
                        buff[j] = toupper(buff[j]);
                    }
                    cout << "client " << map[i] << " data : " << buff << endl;
                    string str(buff);
                    str = "server data : " + str;
                    Write(sock_fd, str.c_str(), str.size());
                }
                if (++n_ready <= 0) {
                    break;
                }
            }
        }
    }
    Close(listen_fd);
    return 0;
}
