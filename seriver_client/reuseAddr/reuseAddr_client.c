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
int Connect(int sock, struct sockaddr *addr, socklen_t len);
void doClient(int sock, const char *ip, int port);

int Socket(int opt){
	int sock;
	sock = socket(AF_INET, opt, 0);
	if(sock <= 0)
		error("socket");
	return sock;
}

int Connect(int sock, struct sockaddr *addr, socklen_t len){
	int service = connect(sock, addr, len);
	if(service == -1)
		error("connet");
	return service;
}

void doClient(int service, const char *ip, int port){
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_aton(ip, &addr.sin_addr);

	Connect(service, (struct sockaddr *)&addr, sizeof(addr));


	int buf[1024];
	int n;
	fd_set fds, tmp;
	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);
	FD_SET(service, &fds);
	while(1){
		tmp = fds;
		select(service + 1, &tmp, NULL, NULL, NULL);
		if(FD_ISSET(STDIN_FILENO, &tmp)){
			n = read(STDIN_FILENO, buf, sizeof(buf));
			if(n == 0){
				shutdown(service, SHUT_WR);
				FD_CLR(STDIN_FILENO, &fds);
				continue;
			}
			send(service, buf, n, 0);
		}
		if(FD_ISSET(service, &tmp)){
			n = recv(service, buf, sizeof(buf), 0);
			if(n == 0){
				break;
			}
			write(STDOUT_FILENO, buf, n);
		}
	}
	close(service);
}

int main(int argc, char *argv[]){
	if(argc != 3){
		fprintf(stderr, "Usge : %s <port>\n", argv[0]);
		exit(1);
	}

	int service = Socket(SOCK_STREAM);
	doClient(service, argv[1], atoi(argv[2]));

	return 0;
}
