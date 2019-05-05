#ifndef __HTTPD_H__
#define __HTTPD_H__

#include "list.h"
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <pthread.h>

enum{
	FILENAME_LEN = 128,		/* 文件名长度 */ 
	READ_BUFSIZE = 1024,	/* 读缓冲区长度 */ 
	WRITE_BUFSIZE = 1024,	/* 写缓冲区长度 */ 
};


struct httpd{
	SLIST_ENTRY(httpd) next;	// 链表
	
	int sockfd;
	struct sockaddr addr;

	char readbuf[READ_BUFSIZE];
	int readIdx;	/* 读缓冲区中最后一个字节的下一个位置 */ 
	int checkIdx;	/* 当前解析的位置 */ 
	int startLine;	/* 当前解析行的开始位置 */ 

	char writebuf[WRITE_BUFSIZE];
	int writeIdx;	/* 写缓冲区中待发送的字节数 */ 

	int check_state;	/* 状态机当前状态 */ 
	int method;			/* 请求方式 */ 

	char filename[FILENAME_LEN];
	char *url;			/* 目标文件的文件名 */ 
	char *version;		/* http版本号 */ 
	char *host;			/* 主机名 */ 
	int content_len;	/* http消息体长度 */ 
	int keep;			/* content-type 状态 : close, keep-alive */ 

	char *mm_addr;		/* 映射文件的位置 */ 
	struct stat file_stat;	/* 文件的状态 */ 
	struct iovec iv[2];		/* 缓冲区 */ 
	int iv_conut;			/* 写入内存块的数量 */ 
};

void processing(struct httpd *);
int readn(struct httpd *);
int writen(struct httpd *);
void inithttpd(struct httpd *, const int, const struct sockaddr *);
void close_conn(struct httpd *);
void add_event(int , int , int );

extern int epollfd;

#endif
