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

#define EPOLL_MAX 65536

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
	int epollfd;
	char buf[1024];
	struct epoll_event event, evets[EPOLL_MAX];
	event.events = EPOLLIN;
	event.data.fd = service;
	
	epollfd = epoll_create(2);
	if(epoll_ctl(epollfd, EPOLL_CTL_ADD, service, &event) == -1){
		fprintf(stderr, "epoll_ctl error\n");
		exit(1);
	}

	int num;
	while(1){
		num = epoll_wait(epollfd, evets, sizeof(evets), -1);
		if(num == 0)
			continue;
		else if (num < 0){
			fprintf(stderr, "epoll_wait error");
			exit(1);
		}

		for(int i = 0; i < num; ++i){
			if(evets[i].data.fd == service && (evets[i].events & EPOLLIN)){
				client = Accept(service, NULL, NULL);
				event.events = EPOLLIN;
				event.data.fd = client;
				epoll_ctl(epollfd, EPOLL_CTL_ADD, client, &event);
			}
			else if(evets[i].events & EPOLLIN){
				int n = recv(evets[i].data.fd, buf, sizeof(buf), 0);
				if(n <= 0){
					epoll_ctl(epollfd, EPOLL_CTL_DEL, evets[i].data.fd, NULL);
					close(evets[i].data.fd);
					fprintf(stderr, "peer close\n");
				}
				send(evets[i].data.fd, buf, n, 0);	
			}
		}
	}
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
