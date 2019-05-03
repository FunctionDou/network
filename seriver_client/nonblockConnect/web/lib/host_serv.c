#include "client_web.h"

// 返回给定主机名(或地址串) 的基本信息
struct addrinfo* host_serv(const char *hostname, const char *service, int family, int socktype){
	int n;
	struct addrinfo hint, *res;

	bzero(&hint, sizeof(hint));
	hint.ai_flags = AI_CANONNAME;
	hint.ai_family = family;
	hint.ai_socktype = socktype;
	if((n = getaddrinfo(hostname, service, &hint, &res)) != 0){
		gai_strerror(n);
		return NULL;
	}

	return res;
}
