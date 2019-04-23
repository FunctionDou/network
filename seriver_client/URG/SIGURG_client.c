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

	char buf1[] = "12345";
	char buf2[] = "67890";
	write(service, buf1, strlen(buf1));
	send(service, "0", strlen("0"), MSG_OOB);
	send(service, buf2, strlen(buf2), MSG_OOB);

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
