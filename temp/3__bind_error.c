#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>

#define EXIT(msg) do{	\
    perror(msg); \
    exit(-1); \
}while(0)

int Socket(int);
int Bind(int, int, const char *);

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
    if(addr == NULL)
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    else 
	sockaddr.sin_addr.s_addr = inet_addr(addr);
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);

    if(bind(sockfd, (struct sockaddr *)(&sockaddr), sizeof(sockaddr)) != 0)
    {
	if(errno == EADDRINUSE)
	{
	    printf("addr %s error\n", inet_ntoa(sockaddr.sin_addr));
	    EXIT("bind");
	}
	else if(errno == EINVAL)
	{
	    printf("port %d error\n", ntohs(sockaddr.sin_port));
	    EXIT("bind");
	}
    }

    return 0;
}


int main(int argc, char *argv[])
{
    int sockfd;
    if(argc != 3)
	exit(1);
    sockfd = Socket(0);

    Bind(sockfd, atoi(argv[2]), argv[1]);
    while(1)
	;

    exit(EXIT_SUCCESS);
}
