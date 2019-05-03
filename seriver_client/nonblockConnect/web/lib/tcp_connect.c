#include "client_web.h"

// 遍历主机的所有 IP 地址, 直到建立一个 TCP 连接 
int tcp_connect(const char *hostname, const char *service){
	int sockfd, n;
	struct addrinfo hint, *res, *resave;

	bzero(&hint, sizeof(hint));
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	
	if((n = getaddrinfo(hostname, service, &hint, &res)) != 0){
		gai_strerror(n);
		exit(1);
	}

	resave = res;
	// 直到建立连接 或 遍历结束退出
	do{
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if(socket < 0)
			continue;
		
		if(connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
			break;
		close(sockfd);
	}while((res	= res->ai_next) != NULL);
	if(res == NULL){
		fprintf(stderr, "tcp_connect error for %s, %s", hostname, service);
		exit(1);
	}

	freeaddrinfo(resave);
	
	return sockfd;
}
