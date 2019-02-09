/*************************************************************************
  > File Name: 2__connect.c
  > Author: Function_Dou
  > Mail: NOT
  > Created Time: 2019年02月07日 星期四 11时02分00秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define EXIT(msg) do { \
    perror(msg);	\
    exit(-1);	\
}while(0)

int Socket(int protocol)
{
    int sockfd;
    if((sockfd = socket(AF_INET, SOCK_STREAM, protocol)) < 0)
	return -1;
    return sockfd;
}

int Connect(int sockfd, int port, const char *addr)
{
    struct sockaddr_in cliaddr;
    cliaddr.sin_port = htons(port);
    cliaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    cliaddr.sin_family = AF_INET;

    if(connect(sockfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) != 0)
	return -1;
    return 0;
}

int main(int argc, char *argv[])
{
    int sockfd;
    sockfd = Socket(0);
    if(Connect(sockfd, 8080, "127.0.0.1") != 0)
	EXIT("connect");


    exit(EXIT_SUCCESS);
}
