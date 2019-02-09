/*************************************************************************
  > File Name: service_client.c
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
#include <ctype.h>

#define EXIT(msg) do{	\
    perror(msg); \
    exit(-1);	\
}while(0)

// 输出本机与连接端的IP和端口信息
void getinfo(int, const char *);

// 小写转为大写
void Toupper(char *str)
{
    char *p = str;
    while(*p)
    {
	*p = toupper(*p);
	++p;
    }
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

// 输出本机与连接端的IP和端口信息
void getinfo(int sockfd, const char *info)
{
    struct sockaddr_in sockaddr1, sockaddr2;
    socklen_t sock_len1, sock_len2;

    sock_len1 = sizeof(sockaddr1);
    sock_len2 = sizeof(sockaddr2);
    int ret;
    ret = getsockname(sockfd, (struct sockaddr *)&sockaddr1, &sock_len1);
    if(ret < 0) EXIT("getsockname");
    ret = getpeername(sockfd, (struct sockaddr *)&sockaddr2, &sock_len2);
    if(ret < 0) EXIT("getpeername");
    fprintf(stdout, "host IP %s : %d", inet_ntoa(sockaddr1.sin_addr), ntohs(sockaddr1.sin_port));
    fprintf(stdout, "%s %s : %d", info, inet_ntoa(sockaddr1.sin_addr), ntohs(sockaddr1.sin_port));
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
    while(1)
    {
	int n = read(clientfd, buf, sizeof(buf));
	Toupper(buf);
	write(clientfd, buf, n);
    }

    return 0;
}

// 客户端
int client(int port, const char *cli_addr)
{
    int sockfd;
    sockfd = Socket(0);
    Connect(sockfd, port, cli_addr);
    
    // getinfo(sockfd, "service IP");

    char buf[1024];
    while(1)
    {
	int n = read(STDIN_FILENO, buf, sizeof(buf));
	write(sockfd, buf, n);
	n = read(sockfd, buf, sizeof(buf));
	write(STDOUT_FILENO, buf, n);
    }

    return 0;
}

// 输入 1(服务端)或者2(客户端) 端口号 本机IP地址
// 如 : ./a.out 1 8080 192.168.1.16
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
