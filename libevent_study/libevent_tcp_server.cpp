//
// Created by yjs on 2022/1/22.
//

#include "wrap.h"
#include <event.h>
#include <iostream>
#include <unordered_map>

using namespace std;


static const int SERVER_PORT = 8000;
static const char SERVER_IP[] = "127.0.0.1";
static const int ReadBufferSize = 1500;
char buffer[ReadBufferSize];


unordered_map<int, string> map;
struct event *evs[1024];
static int count = 0;


void cfd_callback(int client_fd, short event, void *arg) {

    // 读数据
    int *count_ev = reinterpret_cast<int *>(arg);
    cout << *count_ev << endl;


    memset(buffer, 0, sizeof(buffer));
    ssize_t n = Read(client_fd, buffer, sizeof(buffer));

    if (n <= 0) {
        cerr << "error or connect closed  . . . " << endl;
        // 下树
        event_del(evs[--(*count_ev)]);

    } else {
        cout << "client " << map[client_fd] << " data : " << buffer << endl;
        string str(buffer);
        str = "server data : " + str;
        Write(client_fd, str.c_str(), str.size());
        // 没有使用buffer数据 所以需要memset()
        memset(buffer, 0, sizeof(buffer));
    }
}


void lfd_callback(int lfd, short event, void *arg) {
    // lfd 变化 先提取

    event_base *base = reinterpret_cast<event_base *>(arg);

    sockaddr_in client_address{};
    socklen_t client_len = sizeof(client_address);
    int client_fd = Accept(lfd, (sockaddr *) &client_address, &client_len);
    char ip[INET_ADDRSTRLEN] = "";

    inet_ntop(AF_INET, &client_address.sin_addr.s_addr, ip, 16);
    cout << "new client connect . . . and ip is " << ip << " port is : " << ntohs(client_address.sin_port) << endl;
    map[client_fd] = string(ip) + ":" + to_string(ntohs(client_address.sin_port));

    // 将cfd上树

    struct event *ev = event_new(base, client_fd, EV_READ | EV_PERSIST, cfd_callback, &count);
    evs[count] = ev;
    count++;
    event_add(ev, nullptr);
}


int main() {

    // 创建套接字
    // 绑定
    int lfd = tcp4bind(SERVER_PORT, SERVER_IP);
    // 监听
    Listen(lfd, 128);
    // 创建event_base根节点
    event_base *base = event_base_new();
    // 初始化上树节点
    event *ev = event_new(base, lfd, EV_READ | EV_PERSIST, lfd_callback, base);
    // 上树
    event_add(ev, nullptr);
    // 循环监听
    event_base_dispatch(base);// 阻塞
    // 收尾
    event_base_free(base);
    return 0;
}