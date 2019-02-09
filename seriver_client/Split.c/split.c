/*************************************************************************
  > File Name: split.c
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

// 一次性发送一个很大的数据, 大于MTU的值, 该数据会被切分放入不同的分组中
#define N 100000
char buf[N];

// 随机产生buf个大小的数据
void Rand()
{
    for(int i = 0; i < N; i++)
	buf[i] = rand() % 128 + 1;
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

    int n;
    do{
        n = recv(clientfd, buf, sizeof(buf), 0);
	send(clientfd, buf, n, 0);
    }while(n);

    close(clientfd);
    close(sockfd);

    return 0;
}

// 客户端 : 将数据发送给对端, 默认接收但不输出
int client(int port, const char *cli_addr)
{
    Rand();
    int sockfd;
    sockfd = Socket(0);
    Connect(sockfd, port, cli_addr);
    
    // 只发不接
    send(sockfd, buf, sizeof(buf), 0);

    int n = 0;
    do
    {
	n = recv(sockfd, buf, sizeof(buf), 0);
    }while(1);

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
