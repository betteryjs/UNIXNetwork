//
// Created by yjs on 2022/1/4.
//

// 1. socket() man 7 socket
// 2. connect() man connect
// 3. write()
// 4. read()
// 5. close()


#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

int main(){

    // 创建套接字
    int sock_fd;
    sock_fd=socket(AF_INET,SOCK_STREAM,0);
    // 连接服务器  nc -l -p 8080
    struct sockaddr_in addr;
    addr.sin_family=AF_INET;
    addr.sin_port= htons(8080);
    inet_pton(AF_INET,"192.168.2.249",&addr.sin_addr.s_addr);

    connect(sock_fd,(struct sockaddr *)&addr, sizeof(addr));

    const int bufferSize=1024;
    char buff[bufferSize];

    // 读写数据
    while (true){
        int n=read(STDIN_FILENO,buff, sizeof(buff));  // 读数据
        write(sock_fd,buff,n); // 发送数据给server
        if(*buff=='q'){
            break;
        }
        cout<< "server massage : ";
        // 同步 阻塞
        n= read(sock_fd,buff, sizeof(buff));
        write(STDOUT_FILENO,buff,n);

        if(*buff=='q'){
            break;
        }
    }
    // 关闭
    close(sock_fd);
    return 0;
}

