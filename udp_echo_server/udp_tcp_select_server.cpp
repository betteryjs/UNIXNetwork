//
// Created by yjs on 2022/1/30.
//

#include "wrap.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>

using namespace std;

const unsigned short SERVER_PORT = 8000;
const int BUFFER_SIZE = 1500;
const char SERVER_IP[] = "0.0.0.0";

void free_process(int sig) {
  pid_t pid;
  while (true) {
    pid = waitpid(-1, nullptr, WNOHANG);
    if (pid <= 0) {
      // pid<0子进程全部退出
      // pid=0  没有进程退出
      break;
    } else {
      cout << "回收了子进程 pid is :" << pid << endl;
    }
  }
}

int main() {

  int listen_fd, udp_fd;
  int on = 1;

  sockaddr_in server_address;
  unordered_map<int, string> tcp_map;

  /// Tcp server
  listen_fd = Socket(AF_INET, SOCK_STREAM, 0);

  bzero(&server_address, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(SERVER_PORT);
  inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr.s_addr);

  // 设置端口复用
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  Bind(listen_fd, (sockaddr *)&server_address, sizeof(server_address));

  Listen(listen_fd, 128);

  /// Udp Server

  udp_fd = Socket(AF_INET, SOCK_DGRAM, 0);
  bzero(&server_address, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(SERVER_PORT);
  inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr.s_addr);
  Bind(udp_fd, (sockaddr *)&server_address, sizeof(server_address));

  // 注册 监听信号
  signal(SIGCHLD, free_process);

  fd_set old_set, r_set;
  FD_ZERO(&old_set);
  FD_ZERO(&r_set);
  // 将lfd添加到old_set中
  FD_SET(listen_fd, &old_set);
  FD_SET(udp_fd, &old_set);

  int maxfd = max(listen_fd, udp_fd); // 最大的文件描述符
  //  int maxfd = listen_fd;// 最大的文件描述符
  while (true) {

    r_set = old_set; //将老的old_set 赋值给需要监听的集合中
    int n = select(maxfd + 1, &r_set, nullptr, nullptr, nullptr);
    if (n < 0) {
      perror("select error");
      break;
    } else if (n == 0) {
      continue; // 如果没有变化，重新监听
    } else {

      // 监听到了文件描述符号的变化
      // lfd 变化

      if (FD_ISSET(listen_fd, &r_set)) {
        // 提取
        sockaddr_in client_address;
        socklen_t len = sizeof(client_address);
        char ip[16] = "";
        // 提取新的连接
        int conn_fd = Accept(listen_fd, (sockaddr *)&client_address, &len);
        inet_ntop(AF_INET, &client_address.sin_addr.s_addr, ip, 16);
        cout << "new client connect . . . and ip is " << ip
             << " port is : " << ntohs(client_address.sin_port) << endl;
        // 将cfd添加至oldest集合中，以下次监听
        tcp_map[conn_fd] =
            string(ip) + ":" + to_string(ntohs(client_address.sin_port));

        FD_SET(conn_fd, &old_set);
        // 更新maxfd
        if (conn_fd > maxfd) {
          maxfd = conn_fd;
        }
        // 如果只有lfd变化 continue
        if (--n == 0) {
          continue;
        }
      }

      // 处理提取的lfd
      // cfd 变化 遍历lfd之后的文件描述符是否
      for (int i = 0; i <= maxfd; ++i) {

        // 如果i文件描述符在r_set中
        if (FD_ISSET(i, &r_set)) {

          char buffer[1500] = "";

          ssize_t ret_n = Read(i, buffer, sizeof(buffer));

          if (ret_n < 0) {
            // error 将cfd关闭 从oldset中删除
            cerr << "error !!! " << endl;
            close(i);
            FD_CLR(i, &old_set);
          } else if (ret_n == 0) {

            cout << "client [ " << tcp_map[i] << " ] aborted connection"
                 << endl;

            close(i);
            FD_CLR(i, &old_set);
          } else {
            cout << "client " << tcp_map[i] << " data : " << buffer << endl;
            string str(buffer);
            str = "server data : " + str;
            write(i, str.c_str(), str.size());
          }
        }
      }

      if (FD_ISSET(udp_fd, &r_set)) {

        char UDP_Buffer[BUFFER_SIZE];
        sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);
        ssize_t n = recvfrom(udp_fd, UDP_Buffer, BUFFER_SIZE, 0,
                             (sockaddr *)&client_address, &client_address_len);

        // 一直阻塞 直到client发来数据

        if (n < 0) {
          perror("n is 0");
          break;
        } else {

          string send_string;

          send_string.clear();
          cout << "client date : " << UDP_Buffer << endl;
          send_string = "server date : " + string(UDP_Buffer);
          sendto(udp_fd, send_string.c_str(), send_string.size(), 0,
                 (sockaddr *)&client_address, client_address_len);
        }
      }
    }
  }

  return 0;
}