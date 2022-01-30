#include "wrap.h"
#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

static int count;
const unsigned short int SERV_PORT = 8000;
const int MAX_LINE = 4096;
const int NDG = 2000;
const int DGLEN = 1400;
void dg_echo(int, sockaddr *, socklen_t);
void dg_cli(FILE *, int, const sockaddr *, socklen_t);

void dg_echo(int sockfd, sockaddr *pcliaddr, socklen_t clilen) {
  socklen_t len;
  char mesg[MAX_LINE];

  signal(SIGINT, [](const int signo) {
    printf("\nreceived %d datagrams\n", count);
    exit(0);
  });

  for (;;) {
    len = clilen;
    recvfrom(sockfd, mesg, MAX_LINE, 0, pcliaddr, &len);

    count++;
  }
}

void dg_cli(FILE *fp, int sockfd, const sockaddr *pservaddr,
            socklen_t servlen) {
  int i;
  char sendline[DGLEN];

  for (i = 0; i < NDG; i++) {
    sendto(sockfd, sendline, DGLEN, 0, pservaddr, servlen);
  }
}

int main(int argc, char **argv) {
  int sockfd;
  sockaddr_in servaddr;

  if (argc != 2)
    perror("usage: udpcli <IPaddress>");

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(SERV_PORT);
  inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

  sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

  dg_cli(stdin, sockfd, (sockaddr *)&servaddr, sizeof(servaddr));

  exit(0);
}
