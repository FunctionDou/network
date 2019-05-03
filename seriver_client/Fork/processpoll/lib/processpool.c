#include "processpool.h"
#include "run_client.h"

typedef void(*sighandler)(int);

static void init(struct processpool * );
static void run_paren(struct processpool *);	// 父进程处理
static void run_client(struct processpool *);	// 子进程处理
static void setup_sig_pipe(struct processpool *);	// 信号设置

static int setnonblock(int );
static void add_event(int , int );
static void del_event(int , int );
static void sig_handler(int );
static void add_sig(int , sighandler );

int sig_pipefd[2];

// 设置文件描述符为非阻塞
int setnonblock(int fd){
	int old_fd;

	old_fd = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, old_fd | O_NONBLOCK);

	return old_fd;
}

// 在 epoll 中注册事件, 并设置文件描述符为非阻塞
void add_event(int epollfd, int fd){
	struct epoll_event event;

	event.events = EPOLLIN;
	event.data.fd = fd;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	setnonblock(fd);
}

// 从 epoll 中删除事件
void del_event(int epollfd, int fd){
	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
	close(fd);
}

// 信号处理函数 : 主要完成向管道中写入中断号通知 epoll , 让信号与就绪事件一起被处理
void sig_handler(int signo){
	int err;
	int msg;

	msg = signo;
	// 如果输入操作被中断打断, 结束后将 errno 恢复
	err = errno;
	write(sig_pipefd[1], (char *)&msg, sizeof(msg));
	errno = err;
}

// 注册信号
void add_sig(int signo, sighandler handle){
	struct sigaction action;

	action.sa_flags = 0;
	action.sa_handler = handle;
	sigemptyset(&action.sa_mask);
	action.sa_flags |= SA_RESTART;
	if(sigaction(signo, &action, NULL) != 0){
		fprintf(stderr, "sigaction error");
		return;
	}
}


// 初始化 processpool 结构体
void init(struct processpool * init){
	init->stop = 0;
	init->pool_id = -1;

	for(int i = 0; i < MAX_PROCESSPOOL; ++i){
		// 因为是主机进程间的通信, 所有协议族应该使用 XX_UNIX 
		socketpair(PF_UNIX, SOCK_STREAM, 0, init->sub_process[i].pipe);
		pid_t pid = init->sub_process[i].pid = fork();
		if(pid > 0){
			close(init->sub_process[i].pipe[1]);	// 父进程关闭读端
			printf("id = %d, pid = %d\n", i, pid);
			continue;
		}
		else if(pid == 0){
			close(init->sub_process[i].pipe[0]);	// 子进程关闭写端
			init->pool_id = i;	// 子进程保存所在进程数组中的 id 
			break;
		}
	}
}

// 执行监听和处理, 其实就是执行父进程
void run(struct processpool * pool){
	init(pool);
	if(pool->pool_id != -1){
		run_client(pool);
		return;
	}
	run_paren(pool);
}

// 初始化管道并设置信号处理函数
void setup_sig_pipe(struct processpool * pool){

	// 因为是主机进程间的通信, 所有协议族应该使用 XX_UNIX 
	socketpair(PF_UNIX, SOCK_STREAM, 0, sig_pipefd);
	setnonblock(sig_pipefd[1]);
	add_event(pool->epoll_fd, sig_pipefd[0]);

	add_sig(SIGINT, sig_handler);
	add_sig(SIGCHLD, sig_handler);
	add_sig(SIGPIPE, sig_handler);
}

// 父进程处理函数 : 
// 主要就是实现监听事件, 如果是就绪文件描述符则交给进程处理
// 如果是信号, 则处理信号(这里就是将信号通过管道与文件描述符联系起来的)
void run_paren(struct processpool * pool){
	int epollfd = pool->epoll_fd = epoll_create(1);
	setup_sig_pipe(pool);
	add_event(epollfd, pool->listen_fd);

	int n;
	int next_id = 0;
	int informClient = 1;
	pid_t pid;
	struct epoll_event evs[MAX_EPOLL_PROCESS];

	while(!pool->stop){
		n = epoll_wait(epollfd, evs, MAX_EPOLL_PROCESS, -1);
		if(n < 0 && errno != EINTR){
			fprintf(stderr, "epll_wait error");
			break;
		}
		for(int i = 0; i < n; ++i){
			int fd = evs[i].data.fd;
			// 有连接就绪
			if(fd == pool->listen_fd && (evs[i].events & EPOLLIN)){
				// 从进程中寻找一个进程
				int id = next_id;
				do{
					if(pool->sub_process[id].pid != -1)
						break;
					id = (id + 1) % MAX_PROCESSPOOL;
				}while(id != next_id);
				if(pool->sub_process[id].pid == -1){
					pool->stop = 1;
					break;
				}

				write(pool->sub_process[id].pipe[0], (char *)&informClient, sizeof(informClient));
				next_id = (id + 1) % MAX_PROCESSPOOL;
				printf("send request to child %d\n", id);
			}
			else if((fd == sig_pipefd[0]) && (evs[i].events & EPOLLIN)){
				char buf[BUFSIZE];
				int ret;
				ret = read(sig_pipefd[0], buf, sizeof(buf));
				for(int j = 0; j < ret; ++j){
					switch (buf[j]){
						// 父进程终止, 杀死所有子进程后退出
						case SIGINT:
							for(int i = 0; i < MAX_PROCESSPOOL; ++i)
								if(pool->sub_process[i].pid != -1)
									kill(pool->sub_process[i].pid, SIGTERM);
							pool->stop = 1;
							printf("parent exit\n");
							break;
						// 子进程退出, 父进程回收资源
						case SIGCHLD:
							while((pid = waitpid(-1, NULL, WNOHANG)) > 0){
								for(int i = 0; i < MAX_PROCESSPOOL; ++i)
									if(pool->sub_process[i].pid == pid){
										printf("client %d exit\n", pid);
										close(pool->sub_process[i].pipe[0]);
										pool->sub_process[i].pid = -1;
									}
							}
							break;
						default:
							break;
					}
				}
			}
		}
	}
	close(epollfd);
}

void run_client(struct processpool * pool){

	int epollfd = pool->epoll_fd = epoll_create(1);
	// 保存子进程与父进程的管道描述符, 以便后面直接使用并直接注册管道监听
	int pipefd = pool->sub_process[pool->pool_id].pipe[1];
	setup_sig_pipe(pool);
	add_event(epollfd, pipefd);

	int n;
	int clientfd;
	struct epoll_event evs[MAX_EPOLL_PROCESS];	/* 一次性处理的就绪文件描述符 */ 
	struct fds userfds[MAX_USER_PROCESS_NUM];	/* 进程能连接的 TCP容量 */ 

	while(!pool->stop){
		n = epoll_wait(epollfd, evs, sizeof(evs), -1);

		for(int i = 0; i < n; ++i){
			int fd = evs[i].data.fd;

			// 如果是父进程通过管道发的消息, 则表示有连接就绪
			if(fd == pipefd && (evs[i].events & EPOLLIN)){
				int retinform;
				int ret;
				ret = read(pipefd, (char *)&retinform, sizeof(retinform));
				if(ret < 0) break;
				clientfd = accept(pool->listen_fd, NULL, NULL);
				if(clientfd  < 0){
					fprintf(stderr, "accept error\n");
					break;
				}
				add_event(epollfd, clientfd);
				printf("accept success, clientfd = %d\n", clientfd);
				// 将连接TCP描述符保存, 以便之后可以直接使用
				fdsinit(&userfds[clientfd], epollfd, clientfd);
			}
			// 子进程处理信号
			else if(fd == sig_pipefd[0] && (evs[i].events & EPOLLIN)){
				char buf[BUFSIZE];
				int ret;
				ret = read(sig_pipefd[0], buf, sizeof(buf));
				for(int j = 0; j < ret; ++j){
					switch(buf[j])
						case SIGTERM:{
						case SIGINT:
							printf("client exit\n");
							pool->stop = 1;
							break;
						default:
							break;
					}
				}
			}
			// 是就绪文件描述符, 就直接调用处理函数即可
			else if(evs[i].events & EPOLLIN){
				if(processing(&userfds[fd]) != 0){
					del_event(epollfd, fd);
				}
			}
		}
	}
	close(epollfd);
	close(pipefd);
}
