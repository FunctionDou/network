#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <errno.h>

#define EXIT(msg) do{	\
    perror(msg); \
    exit(-1);	\
}while(0)

// 输出本机与连接端的IP和端口信息
void getinfo(int, const char *);

// socket 
int Socket(int protocol)
{
    int sockfd;
    if((sockfd = socket(AF_INET, SOCK_STREAM, protocol)) < 0)
	EXIT("socket");
    return sockfd;
}

// bind 
int Bind(int sockfd, int port)
{
    struct sockaddr_in sockaddr;
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr.sin_port = htons(port);
    sockaddr.sin_family = AF_INET;

    if(bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) != 0)
    {
	if(errno == EADDRINUSE)
	    fprintf(stderr, "addr %s erro\n", inet_ntoa(sockaddr.sin_addr));
	else if(errno == EINVAL)
	    fprintf(stderr, "port %d error\n", ntohs(sockaddr.sin_port));
	EXIT("bind");
    }
    return 0;
}

// connect
int Connect(int sockfd, int port, const char *addr)
{
    struct sockaddr_in sockaddr;
    sockaddr.sin_port = htons(port);
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr(addr);
    
    if(connect(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) != 0)
	EXIT("connect");

    printf("connect successful\n");
    return 0;
}

// 服务端
int service(int port)
{
    int sockfd, clientfd;
    sockfd = Socket(0);
    Bind(sockfd, port);
    listen(sockfd, 1);

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    while(1);
    clientfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);

    char buf[1024];
    int n;
    while(1)
    {
	n = recv(clientfd, buf, sizeof(buf), 0);
	if(0 == n)
	    break;
	send(clientfd, buf, n, 0);
    }

    close(clientfd);
    close(sockfd);

    return 0;
}

// 客户端
int client(int port, const char *cli_addr)
{
    int sockfd;
    sockfd = Socket(0);
    Connect(sockfd, port, cli_addr);
    
    char buf[1024];
    char ch;
    int n;
    while(1)
    {
	ch = getchar();
	if('E' == ch)
	    break;
	n = send(sockfd, &ch, 1, 0);
	recv(sockfd, buf, sizeof(buf), 0);
	write(STDOUT_FILENO, buf, n);
    }

    close(sockfd);

    return 0;
}

// 输入 1(服务端)或者2(客户端) 端口号 本机IP地址
int main(int argc, char *argv[])
{
    int i = atoi(argv[1]);

    if(i == 1)
	service(atoi(argv[2]));
    else if(2 == i)
	client(atoi(argv[3]), argv[2]);

    exit(EXIT_SUCCESS);
}
