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
    exit(0);
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
    
    // 客服端通过将IO分离解决阻塞问题
    pid_t pid;
    if((pid = fork()) < 0){
	fprintf(stderr, "fork error\n");
	exit(1);
    }
    else if(pid == 0)
	Write(sockfd);	// 写

    Read(sockfd);   // 读


    return 0;
}

// 输入 1(服务端)或者2(客户端) 端口号 本机IP地址
int main(int argc, char *argv[])
{
    struct sigaction action;
    action.sa_handler = sighandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if(sigaction(SIGCHLD, &action, NULL) != 0)
	EXIT("sigaction");

    client(atoi(argv[2]), argv[1]);

    exit(EXIT_SUCCESS);
}
