//
// Created by 黎北辰 on 2022/1/31.
//
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <memory>
#include <netinet/tcp.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <unistd.h>

using namespace std;

const int events_wait_time = -1;
const int buffer_size = 18;
unsigned short port = 8000;

//// ////////////
int accept_cb(int, int, void *);
int send_cb(int, int, void *);
int recv_cb(int, int, void *);

////////////////

struct sock_item {
  int sock_fd;
  int (*callback)(int fd, int events, void *arg);
  string recv_buffer; // 半包数据 全局存
  string send_buffer;
};

// main loop/event loop

struct reactor {

  int epfd = 0;
  int events_size = 1024;
  epoll_event events[1024];
};

shared_ptr<reactor> eventloop = nullptr;

int send_cb(int fd, int events, void *arg) {
  sock_item *si = reinterpret_cast<sock_item *>(arg);
  string tmp_string{"hello\n"};
  send(fd, tmp_string.c_str(), tmp_string.size(), 0);

  epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  si->sock_fd = fd;
  si->callback = recv_cb;
  ev.data.ptr = si;

  epoll_ctl(eventloop->epfd, EPOLL_CTL_MOD, fd, &ev);

  return 0;
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
    cout << "Rev: " << str_buff << " , " << ret << " Bytes" << endl;

    //        send(fd,buffer, sizeof(buffer),0);
    //        memset(buffer, 0, sizeof(buffer));

    epoll_event ev;
    ev.events = EPOLLOUT | EPOLLET;
    si->sock_fd = fd;
    si->callback = send_cb;
    ev.data.ptr = si;

    epoll_ctl(eventloop->epfd, EPOLL_CTL_MOD, fd, &ev);
  }
  return 0;
}

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

  epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  ev.data.ptr = si;

  epoll_ctl(eventloop->epfd, EPOLL_CTL_ADD, clientfd, &ev);

  return 0;
}

int main(int argc, char **argv) {

  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    return -1;
  }

  int opt = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  eventloop = make_shared<reactor>();
  eventloop->epfd = epoll_create(1);

  sockaddr_in addr;
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
