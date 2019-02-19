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
#include <sys/select.h>
#include <string.h>
#include <sys/epoll.h>


#define EXIT(msg) do{	\
    perror(msg); \
    exit(-1);	\
}while(0)

#define EPOLL_MAX 1024

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

// accept
int Accept(int servicefd, struct sockaddr_in *client_addr, socklen_t *client_len)
{
    int clientfd;
    while(1)
    {
	clientfd = accept(servicefd, (struct sockaddr *)client_addr, client_len);
	// accept被信号打断后重新运行
	if(clientfd < 0)
	{
	    if(errno == EINTR)
		continue;
	    else 
		EXIT("accept");
	}
	break;
    }
    return clientfd;
}

void doService(int servicefd)
{
    int clientfd, epfd;
    char buf[2];

    struct epoll_event event, evts[EPOLL_MAX];
    event.events = EPOLLIN;
    event.data.fd = servicefd;
    epfd = epoll_create(1);
    epoll_ctl(epfd, EPOLL_CTL_ADD, servicefd, &event);

    int n, eventNum;
    while(1)
    {
	// 这里设置的永久阻塞
	eventNum = epoll_wait(epfd, evts, EPOLL_MAX, -1);
	if(eventNum == 0)
	    continue;
	else if(eventNum < 0)
	    EXIT("epoll_wait");
	// 遍历多少个描述符状态改变
	for(int i = 0; i < eventNum; i++)
	{
	    if(evts[i].data.fd == servicefd && (evts[i].events &EPOLLIN))
	    {
		clientfd = Accept(servicefd, NULL, NULL);
		event.events = EPOLLIN;
		event.data.fd = clientfd;
		epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &event);
	    }
	    else if(evts[i].events & EPOLLIN)
	    {
		n = recv(evts[i].data.fd, buf, sizeof(buf), 0);
		if(n <= 0)
		{
		    // 清除关闭的套接字
		    epoll_ctl(epfd, EPOLL_CTL_DEL, evts[i].data.fd, NULL);
		    close(evts[i].data.fd);
		    fprintf(stderr, "peer close\n");
		}
		send(evts[i].data.fd, buf, n, 0);
	    }
	}
    }
}

// 服务端
int service(int port, const char *ser_addr)
{
    int sockfd;
    sockfd = Socket(0);
    Bind(sockfd, port, ser_addr);
    listen(sockfd, 1);

    doService(sockfd);


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
    FD_SET(STDIN_FILENO, &rfds);
    FD_SET(sockfd, &rfds);
    while(1)
    {
	testrfds = rfds;
	select(sockfd + 1, &testrfds, NULL, NULL, NULL);
	if(FD_ISSET(STDIN_FILENO, &testrfds))
	{
	    n = read(STDIN_FILENO, buf, sizeof(buf));
	    if(n == 0)
		break;
	    send(sockfd, &buf, n, 0);
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
