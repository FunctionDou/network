#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>

#define MAX_EVENT 1024
#define BUFSIZE 1024

int setnonblock(int);
void setevent(int, int, int);
void reset_event(int, int);
void * workecho(void *);

struct eventfds{
	int sockfd;
	int epollfd;
};

// 设置文件描述符为非阻塞
int setnonblock(int fd){
	int oldfd;

	oldfd = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, oldfd | O_NONBLOCK);

	return oldfd;
}

// 将文件描述符添加到 epoll 监听事件中, 并设置文件描述符为非阻塞
void setevent(int eventfd, int fd, int isOneshot){
	struct epoll_event event;
	
	event.data.fd = fd;
	event.events = EPOLLIN;
	if(isOneshot)
		event.events |= EPOLLONESHOT;
	epoll_ctl(eventfd, EPOLL_CTL_ADD, fd, &event);

	setnonblock(fd);
}

// 将事件重置成可执行
void reset_event(int eventfd, int fd){
	struct epoll_event event;

	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLONESHOT;
	epoll_ctl(eventfd, EPOLL_CTL_MOD, fd, &event);
}

// 回射线程
void *workecho(void *arg){

	char buf[BUFSIZE];
	int n;
	int sockfd, epollfd;
	struct eventfds *fds; 

	fds = (struct eventfds *)arg;
	sockfd = fds->sockfd;
	epollfd = fds->epollfd;

	while(1){
		n = recv(sockfd, buf, sizeof(buf), MSG_WAITALL);
		if(n == 0){
			close(sockfd);
			printf("client close\n");
			epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, NULL);
			break;
		}
		else if(n < 0){
			// 如果一秒内没有数据, 则重新注册事件并退出线程
			if(errno == EAGAIN){
				fprintf(stderr, "read timeout\n");
				reset_event(epollfd, sockfd);
				break;
			}
		}
		else{
			write(sockfd, buf, n);
			sleep(1);	// 睡眠一秒, 代表数据处理过程
		}
	}
	printf("exit\n");
	pthread_exit((void *)0);
}

int main(int argc, char *argv[]){
	int servfd, clifd;
	int epollfd;
	socklen_t len;
	struct eventfds fds;
	pthread_t tid;
	struct sockaddr_in seraddr, cliaddr;
	struct epoll_event evs[MAX_EVENT];

	seraddr.sin_family = AF_INET;
	seraddr.sin_port = htons(atoi(argv[1]));
	seraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	len = sizeof(cliaddr);
	
	servfd = socket(AF_INET, SOCK_STREAM, 0);
	if(servfd < 0)
		goto exit;
	if(bind(servfd, (struct sockaddr *)&seraddr, sizeof(seraddr)) < 0)
		goto exit;
	listen(servfd, 5);

	epollfd = epoll_create(1);
	setevent(epollfd, servfd, 0);	// servfd 监听描述符不能设置为一次性执行

	int n;
	while(1){
		n = epoll_wait(epollfd, evs, sizeof(evs), -1);

		for(int i = 0; i < n; ++i){
			if(evs[i].data.fd == servfd){
				clifd = accept(servfd, (struct sockaddr *)&cliaddr, &len);
				if(clifd < 0)
					goto exit;
				setevent(epollfd, clifd, 1);
			}
			else if(evs[i].events & EPOLLIN){
				fds.sockfd = clifd;
				fds.epollfd = epollfd;
				// 传入注意 fds 并非是线程安全的
				pthread_create(&tid, NULL, workecho, (void *)&fds);
			}
		}
	}

	close(servfd);
exit:
	exit(0);
}
