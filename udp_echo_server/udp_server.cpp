//
// Created by yjs on 2022/1/21.
//
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>


const short SERVER_PORT = 8000;
//const short CLIENT_PORT=8001;
const int BUFFER_SIZE = 1500;
using namespace std;

int main() {


    // 创建socket
    int sock_fd;

    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    // 绑定
    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    //    server_address.sin_addr.s_addr=inet_addr("127.0.0.1");
    inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr.s_addr);


    int ret = bind(sock_fd, (sockaddr *) &server_address, sizeof(server_address));
    if (ret < 0) {
        perror("");
        return 0;
    }
    string send_string;
    sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);


    char buffer[BUFFER_SIZE];
    while (true) {

        // 读写
        memset(buffer, 0, sizeof(buffer));
        ssize_t n = recvfrom(sock_fd, buffer, BUFFER_SIZE, 0, (sockaddr *) &client_address, &client_address_len);
        // 一直阻塞 直到client发来数据

        if (n < 0) {

            perror("n is 0");
            break;

        } else {

            cout << "client date : " << buffer << endl;
            send_string = "server date : " + string(buffer);
            sendto(sock_fd, send_string.c_str(), send_string.size(), 0, (sockaddr *) &client_address,
                   client_address_len);
            send_string.clear();
        }
    }

    // 关闭
    close(sock_fd);

    return 0;
}