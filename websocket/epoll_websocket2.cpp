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
#include <fcntl.h>
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
  int slength=0;

  //  char recv_buffer[buffer_size]; //
  //  char send_buffer[buffer_size];
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
static int set_nonblock(int);
char *decode_packet(char *, char *, int, int *);
int encode_packet(char *, char *, char *, int);
////////////////

/////////////////////

struct _nty_ophdr {

  unsigned char opcode : 4, rsv3 : 1, rsv2 : 1, rsv1 : 1, fin : 1;
  unsigned char payload_length : 7, mask : 1;

} __attribute__((packed));

struct _nty_websocket_head_126 {
  unsigned short payload_length;
  char mask_key[4];
  unsigned char data[8];
} __attribute__((packed));

struct _nty_websocket_head_127 {

  unsigned long long payload_length;
  char mask_key[4];

  unsigned char data[8];

} __attribute__((packed));

typedef struct _nty_websocket_head_127 nty_websocket_head_127;
typedef struct _nty_websocket_head_126 nty_websocket_head_126;
typedef struct _nty_ophdr nty_ophdr;

////////////////////

char *decode_packet(char *stream, char *mask, int length, int *ret) {

  nty_ophdr *hdr = reinterpret_cast<nty_ophdr *>(stream);
  unsigned char *data =
      reinterpret_cast<unsigned char *>(stream + sizeof(nty_ophdr));
  int size = 0;
  int start = 0;
  // char mask[4] = {0};
  int i = 0;

  // if (hdr->fin == 1) return NULL;

  if ((hdr->mask & 0x7F) == 126) {

    nty_websocket_head_126 *hdr126 = (nty_websocket_head_126 *)data;
    size = hdr126->payload_length;

    for (i = 0; i < 4; i++) {
      mask[i] = hdr126->mask_key[i];
    }

    start = 8;

  } else if ((hdr->mask & 0x7F) == 127) {

    nty_websocket_head_127 *hdr127 = (nty_websocket_head_127 *)data;
    size = hdr127->payload_length;

    for (i = 0; i < 4; i++) {
      mask[i] = hdr127->mask_key[i];
    }

    start = 14;

  } else {
    size = hdr->payload_length;

    memcpy(mask, data, 4);
    start = 6;
  }

  *ret = size;
  umask(stream + start, size, mask);

  return stream + start;
}

int encode_packet(char *buffer,char *mask, char *stream, int length) {

  nty_ophdr head = {0};
  head.fin = 1;
  head.opcode = 1;
  int size = 0;

  if (length < 126) {
    head.payload_length = length;
    memcpy(buffer, &head, sizeof(nty_ophdr));
    size = 2;
  } else if (length < 0xffff) {
    nty_websocket_head_126 hdr = {0};
    hdr.payload_length = length;
    memcpy(hdr.mask_key, mask, 4);

    memcpy(buffer, &head, sizeof(nty_ophdr));
    memcpy(buffer+sizeof(nty_ophdr), &hdr, sizeof(nty_websocket_head_126));
    size = sizeof(nty_websocket_head_126);

  } else {

    nty_websocket_head_127 hdr = {0};
    hdr.payload_length = length;
    memcpy(hdr.mask_key, mask, 4);

    memcpy(buffer, &head, sizeof(nty_ophdr));
    memcpy(buffer+sizeof(nty_ophdr), &hdr, sizeof(nty_websocket_head_127));

    size = sizeof(nty_websocket_head_127);

  }

  memcpy(buffer+2, stream, length);

  return length + 2;
}

int send_cb(int fd, int events, void *arg) {

  sock_item *si = reinterpret_cast<sock_item *>(arg);
  send(fd, si->send_buffer.c_str(), si->send_buffer.size(), 0);
  epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  si->sock_fd = fd;

  si->callback = recv_cb;
  ev.data.ptr = si;
  si->send_buffer.clear();
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

  string linebuf = header_map["Sec-WebSocket-Key"];
  string sha1_data;
  string head;
  head.clear();

  if (!header_map["Sec-WebSocket-Key"].empty()) {

    linebuf += GUID;
    string out = encode::sha1_encode_return_bit(linebuf);

    string sec_accept = base64::base64_encode(out.c_str(), out.size());

    head = "HTTP/1.1 101 Switching Protocols\r\n";
    head += "Upgrade: websocket\r\n";
    head += "Connection: Upgrade\r\n";
    head += "Sec-WebSocket-Accept: " + sec_accept + "\r\n";
    head += "\r\n";

    cout << "response" << endl << endl;
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
  return 0;
}

int recv_cb(int fd, int events, void *arg) {

  sock_item *si = reinterpret_cast<sock_item *>(arg);
  epoll_event ev;

  char buffer[buffer_size];
  memset(buffer, 0, sizeof(buffer));



  int ret = recv(fd, buffer, buffer_size, 0);

  si->recv_buffer = string(buffer);

  if (ret < 0) {

    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // 正常情况
      return 1;
    }

    ev.events = EPOLLIN;
    epoll_ctl(eventloop->epfd, EPOLL_CTL_DEL, fd, &ev);
    close(fd);

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
    //    str_buff=string (buffer);
    //    str_buff=str_buff.substr(0,ret);
    //    si->recv_buffer=str_buff;
    //    cout<< "Rev: " <<  si->recv_buffer<< " , "<< ret<< " Bytes"<<endl;

    if (si->status == WS_HANDSHARK) {
      cout << "request\n" << si->recv_buffer << endl;
      handshake(si, eventloop);
    } else if (si->status == WS_DATATRANSFORM) {
      // recv
      data_tranform(si, eventloop);
    } else if (si->status == WS_DATAEND) {
    }
  }
  return 0;
}

void umask(char *data, int len, char *mask) {
  int i;
  for (i = 0; i < len; i++)
    *(data + i) ^= *(mask + (i % 4));
}

int data_tranform(sock_item *si, shared_ptr<reactor> main_loop) {

  int ret = 0;
  char mask[4] = {0};

  char *data = decode_packet(const_cast<char *>(si->recv_buffer.c_str()), mask,
                             si->recv_buffer.size(), &ret);

  printf("data : %s , length : %d\n", data, ret);
  si->recv_buffer.clear();
  //  string s=decode_packet(&si->recv_buffer, mask, si->recv_buffer.size(),
  //  ret); cout <<" data : "<< s << " length :"<<ret<<endl;



//  char send_data[buffer_size];
//  ret = encode_packet(send_data, mask, data, ret);
//
//  si->slength = ret;
//  si->send_buffer.clear();
//  si->send_buffer = string(send_data);
//
//  struct epoll_event ev;
//  ev.events = EPOLLOUT | EPOLLET;
//  // ev.data.fd = clientfd;
//  si->sock_fd = si->sock_fd;
//  si->callback = send_cb;
//  si->status = WS_DATATRANSFORM;
//  ev.data.ptr = si;
////
//  epoll_ctl(eventloop->epfd, EPOLL_CTL_MOD, si->sock_fd, &ev);

  return 0;
}

int accept_cb(int fd, int events, void *arg) {
  // client_fd=accept()
  // add client_fd to tree
  // accept
  sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(sockaddr_in));
  socklen_t client_len = sizeof(client_addr);

  int clientfd = accept(fd, (sockaddr *)&client_addr, &client_len);
  if (clientfd <= 0) {
    return 1;
  }
  set_nonblock(clientfd);

  char ip[INET_ADDRSTRLEN] = {0};
  printf("rev from %s at port %d\n",
         inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip)),
         ntohs(client_addr.sin_port));

  epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  sock_item *si = (sock_item *)malloc(sizeof(sock_item));

  si->sock_fd = clientfd;
  si->callback = recv_cb;
  si->status = WS_HANDSHARK;
  ev.data.ptr = si;

  epoll_ctl(eventloop->epfd, EPOLL_CTL_ADD, clientfd, &ev);

  return clientfd;
}

static int set_nonblock(int fd) {
  int flags;

  flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0)
    return flags;
  flags |= O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flags) < 0){
    return -1;
  }
  return 0;
}

int main(int argc, char **argv) {

  if (argc < 2) {
    cerr << "parm mast 2 " << endl;
    return 1;
  }
  auto port = static_cast<unsigned short>(atoi(argv[1]));

  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    return 1;
  }

  set_nonblock(listen_fd);


  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(struct sockaddr_in));

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(listen_fd, (struct sockaddr *)&addr, sizeof( sockaddr_in)) <
      0) {
    return 1;
  }

  if (listen(listen_fd, 5) < 0) {
    return 1;
  }

  // 设置端口复用
  int opt = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  // epoll opera
  eventloop = make_shared<reactor>();
  eventloop->epfd = epoll_create(1);

  epoll_event ev;
  ev.events = EPOLLIN;

  sock_item *si = (sock_item *)malloc(sizeof(sock_item));

  si->sock_fd = listen_fd;
  si->callback = accept_cb;
  ev.data.ptr = si;
  epoll_ctl(eventloop->epfd, EPOLL_CTL_ADD, listen_fd, &ev);

  while (true) {

    int nready = epoll_wait(eventloop->epfd, eventloop->events,
                            eventloop->events_size, events_wait_time);

    if (nready < -1) {

      break;
    }
    int i = 0;
    for (i=0; i < nready; ++i) {

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
  close(listen_fd);
  return 0;
}
