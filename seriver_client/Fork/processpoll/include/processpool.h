#ifndef __PROCESSPOOL_H__
#define __PROCESSPOOL_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>


enum {
	MAX_PROCESSPOOL = 10,	/* 线程池个数 */ 
	MAX_USER_PROCESS_NUM = 65535,	/* 子进程处理事件的个数 */ 
	MAX_EPOLL_PROCESS = 10000,	/* epoll 一次性能够处理的事件数 */ 
};

struct process{
	pid_t pid;	/* 当前进程ID */
	int pipe[2];	/* 管道, 用来统一事件源 */ 
};

struct processpool{
	int pool_id;	/* 进程 ID */ 
	int epoll_fd;	/* epoll 文件描述符 */ 
	int listen_fd;	/* 监听文件描述符 */ 
	int stop;	/* 进程状态 */ 
	struct process sub_process[MAX_PROCESSPOOL];	
};

extern int sig_pipefd[2];

// 程序运行
void run(struct processpool *);

#endif
