#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

#define EXIT(msg) do{	\
    perror(msg); \
    exit(-1);	\
}while(0)

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

    char buf[1024];
    int n;
    pid_t pid;
    while(1)
    {
	clientfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
	// accept被信号打断后重新运行
	if(clientfd < 0)
	{
	    if(errno == EINTR)
		continue;
	    else 
		EXIT("accept");
	}

	if((pid = fork()) < 0)
	    EXIT("fork");
	else if(0 == pid)
	{
	    close(sockfd);
	    while(1)
	    {
		n = recv(clientfd, buf, sizeof(buf), 0);
		if(0 == n)
		    break;
		send(clientfd, buf, n, 0);
	    }
	    close(clientfd);
	    exit(0);
	}
	close(clientfd);
    }

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
    int n;

    fd_set rfds, testrfds;
    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);
    FD_SET(STDIN_FILENO, &rfds);
    while(1)
    {
	testrfds = rfds;
	if(select(sockfd + 1, &testrfds, NULL, NULL, NULL) < 0)
	    EXIT("select");

	if(FD_ISSET(STDIN_FILENO, &testrfds))
	{
	    n = read(STDIN_FILENO, buf, sizeof(buf));
	    if(n == 0)
		break;
	    send(sockfd, &buf, strlen(buf), 0);
	}
	if(FD_ISSET(sockfd, &testrfds))
	{
	    n = recv(sockfd, buf, sizeof(buf), 0);
	    if(n == 0)
		break;
	    write(STDOUT_FILENO, buf, n);
	}
    }
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
