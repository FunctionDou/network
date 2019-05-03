#include "run_client.h"

void fdsinit(struct fds *init, int epollfd, int sockfd){
	init->epollfd = epollfd;
	init->sockfd = sockfd;
}

// 只是简单的实现了回射, 没有实现内置缓存
int processing(struct fds *pro){
	int n;
	char buf[BUFSIZE];
	n = read(pro->sockfd, buf, BUFSIZE);
	if(n == 0)
		return -1;
	write(pro->sockfd, buf, n);

	return 0;
}
