#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/types.h>
#include <fcntl.h>

#define error(msg) do{	\
	perror(msg); \
	exit(1); \
}while(0)

#define EPOLL_MAX 1024

int Socket(int opt);
int timeout_connect(int sockfd, struct sockaddr *addr, socklen_t socklen, int nsec);
void doClient(int sock, const char *ip, int port, int nsec);
int setnonblock(int fd);

int setnonblock(int fd){
	int oldfd;
	oldfd = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, oldfd | O_NONBLOCK);
	return oldfd;
}

int Socket(int opt){
	int sock;
	sock = socket(AF_INET, opt, 0);
	if(sock <= 0)
		error("socket");
	return sock;
}

// connect 超时封装
int timeout_connect(int sockfd, struct sockaddr *addr, socklen_t socklen, int nsec){
	int oldfd;
	int ret;
	int err = 0;	// 通过 getsockopt 获取 connect 是否错误
	struct timeval tval;	// 设置定时时间

	// 设置为非阻塞
	oldfd = setnonblock(sockfd);
	if((ret = connect(sockfd, addr, socklen)) < 0){
		if(errno != EINPROGRESS)
			return -1;
	}
	// 本机连接, 会立即建立连接
	if (ret == 0){
		goto done;
	}
	
	// rset : 用于判断可读; wset : 用于判断可写.
	fd_set rset, wset;
	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);
	wset = rset;

	tval.tv_sec = nsec;
	tval.tv_usec = 0;
	if(select(sockfd+1, &rset, &wset, NULL, nsec ? &tval : 0) == 0){
		// 超时
		close(sockfd);
		errno = ETIMEDOUT;
		return -1;
	}
	// 可读 | 可写
	if(FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)){
		socklen_t len = sizeof(err);
		// 通过 getsockopt 不返回0, 表示连接出错
		if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
			return -1;
	}
	else{
		fprintf(stderr, "select error");
		exit(1);
	}

done:
	// sockfd 恢复到进函数进前的状态. 因为只有 connect 需要非阻塞
	fcntl(sockfd, F_SETFL, oldfd);
	if(err){
		close(sockfd);
		errno = err;
		return -1;
	}
	return 0;
}

void doClient(int service, const char *ip, int port, int nsec){
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_aton(ip, &addr.sin_addr);

	// 如果直接连接则退出, 否则一直尝试
	while(1){
		if(timeout_connect(service, (struct sockaddr *)&addr, sizeof(addr), nsec) == 0)
			break;
	}
	fprintf(stderr, "connect successful\n");

	int n;
	int stat = 0;
	char buf[1024];
	fd_set rset, trset;

	FD_ZERO(&rset);
	FD_SET(service, &rset);
	FD_SET(STDIN_FILENO, &rset);
	while(1){
		trset = rset;
		if(select(service + 1, &trset, NULL, NULL, NULL) < 0){
			error("select error");
		}
		if(FD_ISSET(STDIN_FILENO, &trset)){
			n = read(STDIN_FILENO, buf, sizeof(buf));
			if(n == 0){
				stat = 1;
				shutdown(service, SHUT_WR);
				continue;
			}
			send(service, &buf, n, 0);
		}
		if(FD_ISSET(service, &trset)){
			n = recv(service, buf, sizeof(buf), 0);
			if(n == 0){
				if(stat == 1){
					shutdown(service, SHUT_RD);
					return;
				}
				continue;
			}
			write(STDOUT_FILENO, buf, n);
		}
	}
}

int main(int argc, char *argv[]){
	if(argc != 4){
		fprintf(stderr, "Usge : %s <port>\n", argv[0]);
		exit(1);
	}

	int service = Socket(SOCK_STREAM);
	doClient(service, argv[1], atoi(argv[2]), atoi(argv[3]));

	return 0;
}
