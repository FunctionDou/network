/*************************************************************************
  > File Name: Nagle.c
  > Author: Function_Dou
  > Mail: NOT
  > Created Time: 2019年02月07日 星期四 11时15分08秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/tcp.h>

#define EXIT(msg) do{	\
    perror(msg); \
    exit(-1);	\
}while(0)

// 设置套接字选项
void Setsockopt(int sockfd, int optname, int buf)
{
    if(setsockopt(sockfd, IPPROTO_TCP, optname, (const char *)&buf, sizeof(buf)) != 0)
	EXIT("setsockopt");
}

// socket 
int Socket(int protocol)
{
    int sockfd;
    if((sockfd = socket(AF_INET, SOCK_STREAM, protocol)) < 0)
	EXIT("socket");
    return sockfd;
}

// bind 
int Bind(int sockfd, int port, const char *addr)
{
    struct sockaddr_in sockaddr;
    sockaddr.sin_addr.s_addr = inet_addr(addr);
    sockaddr.sin_port = htons(port);
    sockaddr.sin_family = AF_INET;

    if(bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) != 0)
	EXIT("bind");
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
    return 0;
}

// 服务端
int service(int port, const char *ser_addr)
{
    int sockfd, clientfd;
    sockfd = Socket(0);
    Bind(sockfd, port, ser_addr);
    listen(sockfd, 1);

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
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

    // 设置套接字选项, 关闭Nagle算法
    // Setsockopt(sockfd, TCP_NODELAY, 4);

    Connect(sockfd, port, cli_addr);
    
    char buf[1024];
    char ch = '0';
    int n = 1000;

    while(n--)
	send(sockfd, &ch, 1, 0);
    n = recv(sockfd, buf, sizeof(buf), 0);
    write(STDOUT_FILENO, buf, n);

    close(sockfd);

    return 0;
}

// 输入 1(服务端)或者2(客户端) 端口号 本机IP地址
int main(int argc, char *argv[])
{
    if(argc != 4)
	exit(-1);

    int i = atoi(argv[1]);

    if(i == 1)
	service(atoi(argv[2]), argv[3]);
    if(2 == i)
	client(atoi(argv[2]), argv[3]);

    exit(EXIT_SUCCESS);
}
