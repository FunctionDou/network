#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/types.h>

#define BUFSIZE 1024

void *echo(void *sockfd){
	// 使线程分离, 退出后直接释放资源
	pthread_detach(pthread_self());

	char buf[BUFSIZE];
	int n;
	int fd = *(int *)sockfd;

	while(1){
		n = read(fd, buf, sizeof(buf));
		if(0 == n) break;
		write(fd, buf, n);
	}
	close(fd);
	
	pthread_exit((void *)0);
}

int main(int argc, char *argv[]){
	int sockfd, clientfd;
	socklen_t socklen;
	struct sockaddr_in servAddr, cliAddr;
	pthread_t tid;
	
	bzero(&servAddr, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(atoi(argv[1]));
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	socklen = sizeof(cliAddr);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
		goto exit;
	if(bind(sockfd, (struct sockaddr* )&servAddr, sizeof(servAddr)) < 0)
		goto exit;

	listen(sockfd, 5);
	
	while(1){
		clientfd = accept(sockfd, (struct sockaddr *)&cliAddr, &socklen);
		if(clientfd < 0)
			goto exit;
		if(pthread_create(&tid, NULL, echo, (void *)&clientfd))
			goto exit;
	}

exit:
	return 0;
}
