//
// Created by yjs on 2022/1/4.
//

// 1. socket() man 7 socket
// 2. connect() man connect
// 3. write()
// 4. read()
// 5. close()

#include <arpa/inet.h>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

const unsigned short SERVER_PORT = 13;
const char SERVER_IP[] = "139.199.215.251";
int main(int argc, char **argv) {

  // 创建套接字
  int sock_fd;
  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  // 连接服务器  nc -l -p 8080
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(SERVER_PORT);
  inet_pton(AF_INET, SERVER_IP, &addr.sin_addr.s_addr);

  connect(sock_fd, (struct sockaddr *) &addr, sizeof(addr));

  const int bufferSize = 1024;
  char buff[bufferSize];

  // 读写数据
  while (true) {
        memset(buff,0, sizeof(buff));

        ssize_t n = read(sock_fd, buff, sizeof(buff));// 读数据
    if (n>0){
      cout << buff<<endl;
      break;

    } else{
      break;
    }
  }
  // 关闭
  close(sock_fd);
  return 0;
}
