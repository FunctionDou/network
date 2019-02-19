#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define EXIT(msg) do {\
    perror(msg);	\
    exit(-1);	\
}while(0)

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in service_addr;

    service_addr.sin_port = htons(8080);    // 服务端熟知端口
    service_addr.sin_addr.s_addr = inet_addr("192.168.1.16");	// 服务端IP地址
    service_addr.sin_family = AF_INET;

    // 获取套接字
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	EXIT("socket");

    // 建立连接
    if(connect(sockfd, (struct sockaddr *)&service_addr, sizeof(service_addr)) != 0)
	EXIT("connect");

    char buf[1024];
    int n;
    // 发送
    while(1)
    {
	n = read(STDIN_FILENO, buf, sizeof(buf));
	send(sockfd, buf, n, 0);
	n = recv(sockfd, buf, sizeof(buf), 0);
	write(STDOUT_FILENO, buf, n);
    }
    

    exit(EXIT_SUCCESS);
}
