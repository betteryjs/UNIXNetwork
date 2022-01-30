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
const char sock_client_path[] = "sock.client";
const char sock_server_path[] = "sock.server";


int main() {


    unlink(sock_client_path);


    // 创建套接字
    int sock_unix_fd = socket(AF_UNIX, SOCK_STREAM, 0);


    // 如果不绑定就是隐式绑定


    sockaddr_un unix_address;
    unix_address.sun_family = AF_UNIX;
    strcpy(unix_address.sun_path, sock_client_path);



    if (bind(sock_unix_fd, (sockaddr *) &unix_address, sizeof(unix_address)) < 0) {

        perror("bind error ");
        return 0;
    }
    // 连接

    sockaddr_un server_address;
    server_address.sun_family = AF_UNIX;
    strcpy(server_address.sun_path, sock_server_path);

    connect(sock_unix_fd, (sockaddr *) &server_address, sizeof(server_address));

    // 读写
    char buff[BUFF_SIZE] = "";
    string read_str;

    while (true) {
        cout << "client data : ";
        cout.flush();
        getline(cin, read_str);
        send(sock_unix_fd, read_str.c_str(), read_str.size(), 0);
        memset(buff, 0, sizeof(buff));
        ssize_t n = recv(sock_unix_fd, buff, sizeof(buff), 0);


        if (n < 0) {

            cerr << "error . . ." << endl;
            break;
        } else if (n == 0) {

            cerr << "server close . . ." << endl;
            break;

        } else {

            cout << buff << endl;
        }
    }


    // 关闭
    close(sock_unix_fd);


    return 0;
}