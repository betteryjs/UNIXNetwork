//
// Created by 黎北辰 on 2022/1/31.
//
#include "do_string.h"
#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/sha.h>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <netinet/tcp.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <unistd.h>
using namespace std;
using namespace base64;

const int events_wait_time = -1;
const int buffer_size = 1500;
static const string GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

enum WEBSOCKET_STATUS {
  WS_INIT = 0,
  WS_HANDSHARK = 1,
  WS_DATATRANSFORM = 2,
  WS_DATAEND = 3,
};

struct sock_item {
  int sock_fd;
  int (*callback)(int fd, int events, void *arg);
  string recv_buffer{}; // 半包数据 全局存
  string send_buffer{};
  int status;
};

// main loop/event loop

struct reactor {

  int epfd = 0;
  int events_size = 1024;
  epoll_event events[1024];
};

shared_ptr<reactor> eventloop = nullptr;

//// ////////////
int accept_cb(int, int, void *);
int send_cb(int, int, void *);
int recv_cb(int, int, void *);
int handshake(sock_item *, shared_ptr<reactor>);
int data_tranform(sock_item *, shared_ptr<reactor>);
void umask(char *, int, char *);

////////////////

int send_cb(int fd, int events, void *arg) {

  sock_item *si = reinterpret_cast<sock_item *>(arg);
  send(fd, si->send_buffer.c_str(), si->send_buffer.size(), 0);
  epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  si->sock_fd = fd;

  if (si->status == WS_HANDSHARK) {

    si->status = WS_DATATRANSFORM;
  }

  si->callback = recv_cb;
  ev.data.ptr = si;
  epoll_ctl(eventloop->epfd, EPOLL_CTL_MOD, fd, &ev);
  return 0;
}

int handshake(sock_item *si, shared_ptr<reactor> main_loop) {

  vector<string> headers = do_string::split(si->recv_buffer, "\r\n");
  unordered_map<string, string> header_map;
  headers.erase(headers.end() - 2, headers.end());

  string path = headers[0];
  headers.erase(headers.begin());
  for (const string &res : headers) {
    header_map[do_string::split(res, ":")[0]] =
        do_string::trim(do_string::split(res, ":")[1]);
  }
  cout << header_map["Sec-WebSocket-Key"] << endl;

  string linebuf = header_map["Sec-WebSocket-Key"];
  string sha1_data;
  string head;

  if (!header_map["Sec-WebSocket-Key"].empty()) {

    linebuf += GUID;
    string out = encode::sha1_encode_return_bit(linebuf);

    string sec_accept = base64::base64_encode(out.c_str(), out.size());

    head = "HTTP/1.1 101 Switching Protocols\r\n";
    head += "Upgrade: websocket\r\n";
    head += "Connection: Upgrade\r\n";
    head += "Sec-WebSocket-Accept: " + sec_accept + "\r\n";
    head += "\r\n";

    cout << "response" << endl;
    cout << head << endl << endl << endl;

    si->recv_buffer.clear();
    si->send_buffer = head;
    struct epoll_event ev;
    ev.events = EPOLLOUT | EPOLLET;
    // ev.data.fd = clientfd;
    //       si->sock_fd = si->sock_fd;
    si->callback = send_cb;
    si->status = WS_DATATRANSFORM;
    ev.data.ptr = si;

    epoll_ctl(eventloop->epfd, EPOLL_CTL_MOD, si->sock_fd, &ev);
  }
}

int recv_cb(int fd, int events, void *arg) {

  sock_item *si = reinterpret_cast<sock_item *>(arg);
  char buffer[buffer_size];
  memset(buffer, 0, sizeof(buffer));

  string str_buff;
  str_buff.clear();

  int ret = recv(fd, buffer, buffer_size, 0);
  epoll_event ev;
  if (ret < 0) {

    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // 正常情况
    }
    close(fd);

    ev.events = EPOLLIN;
    ev.data.fd = fd;
    epoll_ctl(eventloop->epfd, EPOLL_CTL_DEL, fd, &ev);
    free(si);

  } else if (ret == 0) {
    //
    printf("disconnect %d\n", fd);

    close(fd);

    ev.events = EPOLLIN;
    ev.data.fd = fd;
    epoll_ctl(eventloop->epfd, EPOLL_CTL_DEL, fd, &ev);
    free(si);

  } else {
    str_buff = string(buffer);
    str_buff = str_buff.substr(0, ret);
    si->recv_buffer = str_buff;
    cout << "Rev: " << si->recv_buffer << " , " << ret << " Bytes" << endl;

    if (si->status == WS_HANDSHARK) {
      handshake(si, eventloop);

    } else if (si->status == WS_DATATRANSFORM) {

      // recv
      //      data_tranform(si, move(eventloop));

    } else if (si->status == WS_DATAEND) {

    } else {
    }

    epoll_event ev;
    ev.events = EPOLLOUT | EPOLLET;
    si->sock_fd = fd;
    si->callback = send_cb;
    ev.data.ptr = si;

    epoll_ctl(eventloop->epfd, EPOLL_CTL_MOD, fd, &ev);
  }
  return 0;
}

void umask(char *data, int len, char *mask) {
  int i;
  for (i = 0; i < len; i++)
    *(data + i) ^= *(mask + (i % 4));
}

// int data_tranform(sock_item * si,shared_ptr<reactor> main_loop ){
//
//
//   int ret = 0;
//   char mask[4] = {0};
//   char *data = decode_packet(const_cast<char*>(si->recv_buffer.c_str()),
//   mask, si->recv_buffer.size(), &ret);
//
//
//   printf("data : %s , length : %d\n", data, ret);
//
//   ret = encode_packet(const_cast<char*>(si->send_buffer.c_str()), mask, data,
//   ret);
//
//   si->recv_buffer.clear();
//
//   struct epoll_event ev;
//   ev.events = EPOLLOUT | EPOLLET;
//   //ev.data.fd = clientfd;
//   si->callback = send_cb;
//   si->status = WS_DATATRANSFORM;
//   ev.data.ptr = si;
//
//   epoll_ctl(eventloop->epfd, EPOLL_CTL_MOD, si->sock_fd, &ev);
//
//   return 0;
//
//
//
//
//
//
// }

int accept_cb(int fd, int events, void *arg) {
  // client_fd=accept()
  // add client_fd to tree
  // accept
  sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(sockaddr_in));
  socklen_t client_len = sizeof(client_addr);

  int clientfd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
  if (clientfd <= 0) {
    return 1;
  }

  char ip[INET_ADDRSTRLEN] = {0};
  printf("rev from %s at port %d\n",
         inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip)),
         ntohs(client_addr.sin_port));

  sock_item *si = (sock_item *)malloc(sizeof(sock_item));

  si->sock_fd = clientfd;
  si->callback = recv_cb;
  si->status = WS_HANDSHARK;

  epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  ev.data.ptr = si;

  epoll_ctl(eventloop->epfd, EPOLL_CTL_ADD, clientfd, &ev);

  return 0;
}

int main(int argc, char **argv) {

  if (argc < 2) {
    return -1;
  }

  unsigned short port = atoi(argv[1]);

  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    return -1;
  }

  int opt = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  eventloop = make_shared<reactor>();
  eventloop->epfd = epoll_create(1);

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(struct sockaddr_in));

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) <
      0) {
    return -2;
  }

  if (listen(listen_fd, 128) < 0) {
    return -3;
  }

  // epoll opera

  epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = listen_fd;

  sock_item *si = (sock_item *)malloc(sizeof(sock_item));

  si->sock_fd = listen_fd;
  si->callback = accept_cb;
  si->status = WS_INIT;

  ev.data.ptr = si;
  epoll_ctl(eventloop->epfd, EPOLL_CTL_ADD, listen_fd, &ev);

  while (true) {

    int nready = epoll_wait(eventloop->epfd, eventloop->events,
                            eventloop->events_size, events_wait_time);

    if (nready < -1) {

      break;
    }
    int i = 0;
    for (; i < nready; ++i) {

      if (eventloop->events[i].events & EPOLLIN) {
        sock_item *si = (sock_item *)eventloop->events[i].data.ptr;
        si->callback(si->sock_fd, eventloop->events[i].events, si);
      }
      if (eventloop->events[i].events & EPOLLOUT) {
        sock_item *si = (sock_item *)eventloop->events[i].data.ptr;
        si->callback(si->sock_fd, eventloop->events[i].events, si);
      }
    }
  }
  return 0;
}
