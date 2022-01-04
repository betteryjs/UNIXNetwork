//
// Created by yjs on 2022/1/4.
//

#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

int main(){

    int sock_fd;
    sock_fd=socket(AF_INET,SOCK_STREAM,0);

    struct sockaddr_in addr;
    addr.sin_family=AF_INET;
    addr.sin_port= htons(8080);
    inet_pton(AF_INET,"192.168.2.249",&addr.sin_addr.s_addr);

    // bind
    if (bind(sock_fd,(struct sockaddr *)&addr,sizeof(addr))!=0){
        perror("bind error");
        return 1;
    };


    // int listen(int s, int backlog);
    // man listen
    // s: 监听的套接字
    // backlog： 参数 backlog 指定未完成连接队列的最大长度 128


    listen();



    // accept 没有连接会阻塞
    // man 3 accept
    // int accept(int socket, struct sockaddr *restrict address,
    //           socklen_t *restrict address_len);
    // 功能 从已连接队列提取新的连接
    // 参数: socket
    // address 获取的客户端IP和端口 IPV4 socket struct
    // address_len sizeof( struct socket sockaddr_in)
    // return 新的已连接套接字的文件描述符









    return 0;
}

