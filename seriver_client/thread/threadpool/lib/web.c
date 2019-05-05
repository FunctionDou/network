#include "list.h"
#include "httpd.h"
#include "threadpool.h"


#define MAX_FD 65536
#define MAX_EVENT_NUM 65536

// 需要将数组定义成全局变量, 因为太大会导致栈溢出
struct httpd users[MAX_FD];
struct epoll_event evs[MAX_EVENT_NUM];

// epollfd 是在httpd.h 中声明的全局变量
int epollfd;

int main(int argc, char *argv[]){
	int listenfd, clientfd;
	int ret;
	struct sockaddr_in addr;
	struct threadpool pool;

	initpool(&pool);
	bzero(&addr, sizeof(addr));
	addr.sin_family = PF_INET;
	addr.sin_port = htons(atoi(argv[1]));
	// inet_pton(AF_INET, argv[1], &addr.sin_addr);
	addr.sin_addr.s_addr = htonl((INADDR_ANY));

	listenfd = socket(PF_INET, SOCK_STREAM, 0);
	if(listenfd < 0){
		fprintf(stderr, "socket error\n");
		exit(1);
	}
	ret = bind(listenfd, (struct sockaddr *)&addr, sizeof(addr));
	if(ret < 0){
		fprintf(stderr, "bind error\n");
		exit(1);
	}
	ret = listen(listenfd, 5);
	if(ret < 0){
		fprintf(stderr, "listen error\n");
		exit(1);
	}

	epollfd = epoll_create(1);
	if(epollfd < 0){
		fprintf(stderr, "epoll create error\n");
		exit(1);
	}
	// 将 listen监听描述符加入 epoll 的监听队列中并设置为非阻塞
	add_event(epollfd, listenfd, 0);

	int n;
	while(1){
		n = epoll_wait(epollfd, evs, sizeof(evs), -1);
		if((n < 0) && (errno != EINTR)){
			fprintf(stderr, "epoll_wait error");
			break;
		}

		for(int i = 0; i < n; ++i){
			int fd = evs[i].data.fd;
			if(fd == listenfd){
				socklen_t len;
				struct sockaddr_in addr;
				len = sizeof(addr);
				clientfd = accept(listenfd, (struct sockaddr *)&addr, &len);
				if(clientfd < 0){
					fprintf(stderr, "accept error\n");
					continue;
				}
				init(&users[clientfd], clientfd, (struct sockaddr *)&addr);
			}
			// else if(evs[i].events & (EPOLLHUP | EPOLLIN | EPOLLERR))
			// 不要把 EPOLLRDHUP 写成 EPOLLHUP. 前者是对端关闭, 后者是文件出错
			else if(evs[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
				close_conn(&users[fd]);
			// 读事件就绪 : 
			// readn 将从缓冲区中将 http 请求读入到 httdp 读缓冲区中
			// append 将事件加入到就绪队列中, 由线程池中的线程处理
			else if(evs[i].events & EPOLLIN){
				if(readn(&users[fd]) == 0)
					append(&pool, &users[fd]);
				else 
					close_conn(&users[fd]);
			}
			// 写事件就绪 : 
			// writen 将 http 状态码以及请求的静态文件发送给客户端
			// 如果发送过程中出现问题则断开连接
			else if(evs[i].events & EPOLLOUT){
				if(writen(&users[fd]) == -1)
					close_conn(&users[fd]);
			}
		}
	}

	return 0;
}
