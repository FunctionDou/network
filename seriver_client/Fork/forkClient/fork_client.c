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

void Write(int sockfd){
    int len;
    char buf[1024];

    while(1){
	len = read(STDIN_FILENO, buf, sizeof(buf));
	if(len == 0)
	    break;
	write(sockfd, buf, len);
    }
    close(sockfd);
}

void Read(int sockfd){
    int len;
    char buf[1024];

    while(1){
	len = read(sockfd, buf, sizeof(buf));
	if(len == 0)
	    break;
	write(STDOUT_FILENO, buf, len);
    }
    close(sockfd);
}

// 客户端
int client(int port, const char *cli_addr)
{
    int sockfd;
    sockfd = Socket(0);
    Connect(sockfd, port, cli_addr);

    // 通过创造进程实现IO分离
    pid_t pid;
    if((pid = fork()) < 0){
	fprintf(stderr, "fork error\n");
	exit(1);
    }
    else if(pid == 0)
	Write(sockfd);
    else
	Read(sockfd);

    return 0;
}

// 输入 1(服务端)或者2(客户端) 端口号 本机IP地址
int main(int argc, char *argv[])
{
    if(argc != 4)
	exit(-1);

    struct sigaction action;
    action.sa_handler = sighandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if(sigaction(SIGCHLD, &action, NULL) != 0)
	EXIT("sigaction");

    int i = atoi(argv[1]);
    if(i == 1)
	service(atoi(argv[2]), argv[3]);
    if(2 == i)
	client(atoi(argv[2]), argv[3]);

    exit(EXIT_SUCCESS);
}
