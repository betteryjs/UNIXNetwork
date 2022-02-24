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


const unsigned short SERVER_PORT = 8000;
const unsigned short CLIENT_PORT = 8001;
const int BUFFER_SIZE = 1500;
using namespace std;

int main() {


    // 创建socket

    int sock_fd;
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    // 绑定

    sockaddr_in client_address;
    client_address.sin_family = AF_INET;
    client_address.sin_port = htons(CLIENT_PORT);
    //    server_address.sin_addr.s_addr=inet_addr("127.0.0.1");
    inet_pton(AF_INET, "127.0.0.1", &client_address.sin_addr.s_addr);


    int ret = bind(sock_fd, (sockaddr *) &client_address, sizeof(client_address));

    if (ret < 0) {
        perror("bind error ");
        return 0;
    }


    string client_data;

    sockaddr_in dist_address;
    dist_address.sin_family = AF_INET;
    dist_address.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &dist_address.sin_addr.s_addr);
    socklen_t dist_address_len = sizeof(dist_address);

    //    char* read_data=(char *) malloc(sizeof(char )*BUFFER_SIZE);

    char read_data[BUFFER_SIZE];

    while (true) {


        // send data to server
        cout << "client data : " << flush;
        getline(cin, client_data);
        sendto(sock_fd, client_data.c_str(), client_data.size(), 0, (sockaddr *) &dist_address, dist_address_len);
        client_data.clear();

        memset(read_data, 0, sizeof(read_data));
        ssize_t n = recvfrom(sock_fd, read_data, sizeof(read_data), 0, nullptr, nullptr);

        // read data
        if (n < 0) {

            perror("n is 0");
            break;

        } else if (n > 0) {
            // close
            //            close(sock_fd);
            cout << read_data << endl;
        }
    }


    // 关闭
    close(sock_fd);

    // udp

    return 0;
}