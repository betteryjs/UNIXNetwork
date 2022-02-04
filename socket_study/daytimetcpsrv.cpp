#include <arpa/inet.h>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <time.h>
#include <sys/stat.h>
#include <sys/syslog.h>

using namespace std;
const unsigned short SERVER_PORT = 8001;
const char SERVER_IP[] = "0.0.0.0";

int daemon_init(const string &p_name, const int &facility) {

  pid_t pid;
  pid = fork();
  if (pid < 0) {
    perror("fork");
    return 1;
  }
  if (pid > 0) {
    // father process exit
    exit(0);
  }
  // 2. create new session
  // create new session Break away tty 保证shell（tty）退出时 进程不会退出
  pid = setsid();
  if (pid < 0) {
    perror("setsid");
    return 1;
  }
  // 3. change workspace dir
  int ret = chdir("/");
  if (ret < 0) {
    perror("chdir");
    return 1;
  }
  // 4. set  all mask
  umask(0);
  // 5. Turn off file descriptors
  close(STDERR_FILENO);
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  // 6. do cron work
  // one time to file

  openlog(p_name.c_str(), LOG_PID, facility);

  return 0;
}

int main(int argc, char **argv) {

  daemon_init(argv[0], 0);

  int listenfd, connfd;
  sockaddr_in servaddr;
  time_t ticks;

  listenfd = socket(AF_INET, SOCK_STREAM, 0);

  servaddr.sin_family = AF_INET;

  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

  servaddr.sin_port        = htons(SERVER_PORT);	/* daytime server */
        inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr.s_addr);

	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));


	listen(listenfd, 128);

        while (true) {
		connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);

                ticks = time(NULL);
                string data_str=string(ctime(&ticks));
                write(connfd, data_str.c_str(), data_str.size());
                close(connfd);

	}
}
