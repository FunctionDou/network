#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <signal.h>
#include <fcntl.h>

#define error(msg) do{	\
	perror(msg); \
	exit(1); \
}while(0)

#define EPOLL_MAX 1024

// 因为信号捕捉不能直接传递参数, 所以暂时为设置全局变量
int client;

// 捕捉 SIGURG, 紧急消息
void urg_handler(int signo){
		int len;
		char buf[1024];
		len = recv(client, buf, sizeof(buf), MSG_OOB);
		fprintf(stderr, "URG message %s\n", buf);
		write(STDOUT_FILENO, "a", 1);
		write(STDOUT_FILENO, buf, len);
}

int Socket(int opt);
void Bind(int sock, int port);
void doservice(int server);
int Accept(int sock, struct sockaddr_in *addr, socklen_t *len);

int Socket(int opt){
	int sock;
	sock = socket(AF_INET, opt, 0);
	if(sock <= 0)
		error("socket");
	return sock;
}

void Bind(int sock, int port){
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		error("bind");
}

int Accept(int sock, struct sockaddr_in *addr, socklen_t *len){
	int client;
	client = accept(sock, (struct sockaddr*)addr, len);
	if(client <= 0)
		error("accept");
	return client;
}

void doservice(int service){
	char buf[1024];
	int num;
	
	client = Accept(service, NULL, NULL);

	// 设置套接字拥有者
	if(fcntl(service, F_SETOWN, getpid()) == -1)
		error("fcntl");

	// 注册回调函数
	struct sigaction sig;
	sig.sa_flags = 0;
	sig.sa_handler = urg_handler;
	sigemptyset(&sig.sa_mask);
	if(sigaction(SIGURG, &sig, 0) != 0)
		error("sigaction");
	
	while((num = recv(client, buf, sizeof(buf), 0)) != 0){
		if(num == -1)
			continue;
		write(STDOUT_FILENO, buf, num);
	}

	close(client);
}

int main(int argc, char *argv[]){
	if(argc != 2){
		fprintf(stderr, "Usge : %s <port>\n", argv[0]);
		exit(1);
	}


	int service = Socket(SOCK_STREAM);

	Bind(service, atoi(argv[1]));
	if(listen(service, 1) == -1)
		error("listen");
	
	doservice(service);

	close(service);

	return 0;
}
