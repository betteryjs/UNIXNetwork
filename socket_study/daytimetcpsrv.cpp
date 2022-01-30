#include <arpa/inet.h>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <time.h>

using namespace std;
const unsigned short SERVER_PORT=8001;
const char SERVER_IP[]="0.0.0.0";

int main(int argc, char **argv)
{
	int					listenfd, connfd;
	 sockaddr_in	servaddr;
	time_t				ticks;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	servaddr.sin_family      = AF_INET;

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
