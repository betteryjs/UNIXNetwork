//
// Created by yjs on 2022/1/5.
//

#include "wrap.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>


using namespace std;

// 多进程实现并发服务器
void *client_callback(void *args);

class client_infos {
private:
    int cfd;
    struct sockaddr_in client_address;

public:
    client_infos(int &i, struct sockaddr_in &client_addr) : cfd(i), client_address(client_addr) {}

    client_infos() {}

    void set_cfd(int &i) {
        cfd = i;
    }

    void set_client_address(struct sockaddr_in &addr) {
        client_address = addr;
    }

    int get_cfd() {
        return cfd;
    }

    sockaddr_in get_client_address() {

        return client_address;
    }
};


int main(int argc, char **argv) {

    if (argc < 2) {

        perror("必须传入参数 . . .");
        exit(1);
    }
    // 设置线程分离
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    int lfd = tcp4bind((short) atoi(argv[1]), "192.168.2.249");// 创建绑定 socket
    // 设置端口复用
    int opt=1;
    setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,&opt, sizeof(opt));

    Listen(lfd, 128);
    sockaddr_in client_address;
    socklen_t len = sizeof(client_address);

    while (true) {

      char ip[16] = "";
      int res;
      int cfd = Accept(lfd, (sockaddr *)&client_address, &len);

      /// pthread start
      pthread_t pthread_id;
      client_infos *client_info = new client_infos;
      client_info->set_cfd(cfd);
      client_info->set_client_address(client_address);
      res = pthread_create(&pthread_id, &attr, client_callback, client_info);
      if (res < 0) {

        cerr << "pthread_create failed . . . " << endl;
        exit(1);
      }
      /// pthread end


    }
    exit(0);
}

void *client_callback(void *args) {

    client_infos *info = reinterpret_cast<client_infos *>(args);
    char ip[16] = "";
    const sockaddr_in client_address = info->get_client_address();
    inet_ntop(AF_INET, &client_address.sin_addr.s_addr, ip, 16);
    cout << "new client connect . . . and ip is " << ip << " port is : " << ntohs(client_address.sin_port) << endl;
    char buff[2048];
    while (true) {
        memset(buff, 0, sizeof(buff));
        string str;
        str.clear();
        ssize_t count = read(info->get_cfd(), buff, sizeof(buff));
        if (count < 0) {
            // error
            cerr << "error !!! " << endl;
            break;
        } else if (count == 0) {
            cout << " client close . . ." << endl;
            break;
        } else {

            cout << "client data : " << buff << endl;
            string str(buff);
            str = "server data : " + str;
            write(info->get_cfd(), str.c_str(), str.size());
        }
    }
    close(info->get_cfd());
    delete info;
    return nullptr;
}