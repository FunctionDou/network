/*************************************************************************
  > File Name: socket_bind.c
  > Author: Function_Dou
  > Mail: NOT
  > Created Time: 2019年02月07日 星期四 10时02分08秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>

#define EXIT(msg) do{	\
    perror(msg); \
    exit(-1); \
}while(0)

int Socket(int protocol)
{
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, protocol);
    if(sockfd < 0)
	return -1;

    return sockfd;
}

int Bind(int sockfd, int port, const char *addr)
{
    struct sockaddr_in sockaddr;
    // sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr.sin_addr.s_addr = inet_addr(addr);
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);

    if(bind(sockfd, (struct sockaddr *)(&sockaddr), sizeof(sockaddr)) != 0)
	EXIT("bind");
    return 0;
}

#define __TEST2

int main(int argc, char *argv[])
{

#ifdef __TEST1
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);	// 端口可以不用设置, 随机分配
    addr.sin_addr.s_addr = inet_addr("192.168.1.16");

    socklen_t  socksize;
    socksize = sizeof(addr);

    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(bind(sockfd, (struct sockaddr *)&addr, socksize) != 0)
    {
	printf("bind error\n");
	exit(-1);
    }

    struct sockaddr_in testaddr;
    socklen_t testaddrszie = sizeof(testaddr);
    if(getsockname(sockfd, (struct sockaddr *)(&testaddr), &testaddrszie) != 0)
    {
	printf("getsockname error\n");
	exit(-1);
    }

    printf("%s %d\n", inet_ntoa(testaddr.sin_addr), ntohs(testaddr.sin_port));

#endif

#ifdef __TEST2
    int sockfd; 
    sockfd = Socket(0);
    if(Bind(sockfd, 8080, "192.168.1.16") != 0)
    {
	printf("bind error\n");
	exit(-1);
    }

    struct sockaddr_in addr;
    socklen_t socklen;
    if(getsockname(sockfd, (struct sockaddr *)&addr, &socklen) != 0)
    {
	printf("getsockname error\n");
	exit(-1);
    }
    printf("host IP %s : %d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

#endif

    exit(EXIT_SUCCESS);
}
