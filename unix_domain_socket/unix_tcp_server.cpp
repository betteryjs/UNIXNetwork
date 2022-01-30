//
// Created by yjs on 2022/1/22.
//


#include <arpa/inet.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

using namespace std;


const int BUFF_SIZE = 1500;

const char sock_server_path[] = "sock.server";

int main() {


    unlink(sock_server_path);  // == remove

    // 创建套接字
    int sock_unix_fd = socket(AF_UNIX, SOCK_STREAM, 0);


    sockaddr_un unix_address;
    unix_address.sun_family = AF_UNIX;
    strcpy(unix_address.sun_path, sock_server_path);

    // 绑定
    bind(sock_unix_fd, (sockaddr *) &unix_address, sizeof(unix_address));


    // 监听

    listen(sock_unix_fd, 128);

    // 提取
    sockaddr_un client_unix_address;
    socklen_t client_unix_address_len = sizeof(client_unix_address);
    int client_fd = accept(sock_unix_fd, (sockaddr *) &client_unix_address, &client_unix_address_len);

    cout << "new client file = " << unix_address.sun_path << endl;


    // 读写
    char buff[BUFF_SIZE] = "";
    string write_data;

    while (true) {

        memset(buff, 0, sizeof(buff));
        write_data.clear();

        ssize_t n = recv(client_fd, buff, sizeof(buff), 0);
        if (n <= 0) {
            cout << "err client closed . . ." << endl;
            break;
        } else {

            cout << "client data : " << buff << endl;
            write_data = "server data : " + string(buff);
            send(client_fd, write_data.c_str(), write_data.size(), 0);
        }
    }

    // 关闭
    close(sock_unix_fd);
    close(client_fd);


    return 0;
}