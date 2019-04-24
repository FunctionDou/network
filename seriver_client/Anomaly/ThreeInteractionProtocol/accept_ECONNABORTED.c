#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>

#define EXIT(msg) do{	\
    perror(msg); \
    exit(-1);	\
}while(0)

// SIGCHLD的信号处理
void sighandler(int signo)
{
    if(signo == SIGCHLD)
    {
	pid_t pid;
	// 一定要死循环
	while(1)
	{
	    // waitpid设置为WNOGANG, 非阻塞
	    if((pid = waitpid(-1, NULL, WNOHANG)) <= 0)
		break;
	    printf("child %d terminated\n", pid);
	}
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

    sleep(5);

    char buf[1024];
    int n;
    pid_t pid;
    while(1)
    {
	clientfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
	// accept被信号打断后重新运行
	if(clientfd < 0)
	{
	    if(errno == EINTR){
		continue;
	    }
	    // 改错误依赖操作系统, 至少我的就没有收到
	    else if(errno == ECONNABORTED)
		fprintf(stderr, "peer close\n");
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

    // 设置套接字选项
    struct linger lig;
    lig.l_onoff = 1;
    lig.l_linger = 0;
    // 客户端在 close 的时候不是发送 FIN，而是 RST.
    if(setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (void *)&lig, sizeof(lig)) != 0)
	EXIT("setsockopt");
    // 连接后直接关闭
    Connect(sockfd, port, cli_addr);
    close(sockfd);

    return 0;
}

// 输入 1(服务端)或者2(客户端) 端口号 本机IP地址
int main(int argc, char *argv[])
{
    if(argc != 4)
	exit(-1);
    
    struct sigaction action;
    action.sa_handler = sighandler;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    if(sigaction(SIGCHLD, &action, NULL) != 0)
	EXIT("sig_atomict");

    int i = atoi(argv[1]);
    if(i == 1)
	service(atoi(argv[2]), argv[3]);
    if(2 == i)
	client(atoi(argv[2]), argv[3]);

    exit(EXIT_SUCCESS);
}
