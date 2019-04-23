#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <netinet/in.h>

#define error(msg) do{	\
	perror(msg); \
	exit(1); \
}while(0)

#define EPOLL_MAX 1024

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
	int client;
	char buf[1024];
	// errset 和 terrset 主要用于接收紧急消息
	fd_set rset, trset, errset, terrset;

	client = Accept(service, NULL, NULL);
	FD_ZERO(&rset);
	FD_ZERO(&errset);

	int num;
	int stat = 0;
	while(1){
		FD_SET(client, &rset);
		if(stat == 0)
			FD_SET(client, &errset);
		trset = rset; terrset = errset;

		select(client + 1, &trset, NULL, &terrset, NULL);

		// 接收紧急消息, 当发生错误的时候套接字可读也可写
		if(FD_ISSET(client, &terrset)){
			num = recv(client, buf, sizeof(buf), MSG_OOB);
			buf[num] = 0;
			fprintf(stderr, "OOB message : %s\n", buf);
			FD_CLR(client, &errset);
		}
		if(FD_ISSET(client, &trset)){
			num = recv(client, buf, sizeof(buf), 0);
			if(num == 0)
				break;
			buf[num] = 0;
			write(STDOUT_FILENO, buf, num);
		}
	}
	close(client);
	close(service);
}

int main(int argc, char *argv[]){
	if(argc != 2){
		fprintf(stderr, "Usge : %s <port>\n", argv[0]);
		exit(1);
	}

	int service = Socket(SOCK_STREAM);

	int option = 1;
	int optlen = sizeof(option);
	setsockopt(service, SOL_SOCKET, SO_REUSEADDR, (void *)&option, optlen);

	Bind(service, atoi(argv[1]));
	if(listen(service, 1) == -1)
		error("listen");
	
	doservice(service);

	return 0;
}
