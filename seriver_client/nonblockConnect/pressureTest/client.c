#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSZIE		1024
#define EPOLLSIZE	65536
#define SOCKSIZE	65536

#define F_CONNECTING	1
#define F_CONNECTED		2
#define F_READING		4

int setnonblock(int);
int nonblock_connect();
void initfile(int );

int num;
int epollfd;
int countConn;
int maxfd;
int port;
char *host;

struct file{
	char buf[BUFSZIE];
	int fd;
	int stat;
	socklen_t len;
	struct sockaddr_in addr;
}file[SOCKSIZE];

// 设置成非阻塞
int setnonblock(int fd){
	int oldfd;

	oldfd = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, oldfd | O_NONBLOCK);

	return oldfd;
}

// 初始化每一个连接, 以文件描述符做索引
void initfile(int fd){
	file[fd].fd = fd;
	file[fd].addr.sin_port = htons(port);
	file[fd].addr.sin_family = AF_INET;
	file[fd].len = sizeof(file[fd].addr);
	file[fd].stat = 0;
	inet_aton(host, &file[fd].addr.sin_addr);
}

// 非阻塞连接
int nonblock_connect(){
	int sockfd, oldfd;
	struct epoll_event event;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd >= SOCKSIZE || socket < 0){
		close(sockfd);
		fprintf(stderr, "socket error");
		return -1;
	}

	oldfd = setnonblock(sockfd);
	initfile(sockfd);

done:
	if(connect(sockfd, (struct sockaddr *)&file[sockfd].addr, file[sockfd].len) < 0){
		if(errno != EINPROGRESS){
			printf("%d ", errno);
			close(sockfd);
			perror("connect error");
			exit(1);
		}
		goto done;
	}
	file[sockfd].stat = F_CONNECTING;	// 设置 connect 当前状态, 正在连接中
	event.events = EPOLLIN | EPOLLOUT;
	event.data.fd = sockfd;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &event);

	return 0;
}

int main(int argc, char *argv[]){
	if(argc != 4){
		fprintf(stderr, "usags : %s <host> <port> <connect num>", argv[0]);
		exit(1);
	}
	struct epoll_event evs[EPOLLSIZE];
	
	num = 0;
	countConn = 0;
	host = argv[1];
	port = atoi(argv[2]);
	maxfd = atoi(argv[3]);
	epollfd = epoll_create(2);

	for(int i = 0; i < maxfd; ++i){
		nonblock_connect();
	}


	int n;
	struct epoll_event evt;
	while(1){
		n = epoll_wait(epollfd, evs, sizeof(evs), -1);
		for(int i = 0; i < n; ++i){
			int fd = evs[i].data.fd;
			if(file[fd].stat & F_CONNECTING && (evs[i].events & EPOLLIN || (evs[i].events & EPOLLOUT))){
				int err;
				socklen_t len = sizeof(err);
				// connect 连接失败
				if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0){
					goto done;
				}
				// connect 连接成功,像 fd 写入相同的数据, 修改 socket 只监听读
				file[fd].stat = F_CONNECTED;
				printf("connect %d successful\n", ++countConn);
				evt.data.fd = fd;
				evt.events = EPOLLIN;
				epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &evt);
				send(fd, "hello", 6, 0);
			}
			if(file[fd].stat == F_CONNECTED && (evs[i].events & EPOLLIN)){
				int n = recv(fd, file[fd].buf, BUFSZIE, 0);
				if(n == 0){
					goto done;
				}
				fprintf(stderr, "client %d read\n", fd);
			}
			continue;
done:
			fprintf(stderr, "connect error");
			memset(&file[fd], 0, sizeof(file[fd]));
			close(fd);
			epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
		}
	}
	
	
	exit(0);
}
