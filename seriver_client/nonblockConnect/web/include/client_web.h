#ifndef __CLIENTWEB_H__
#define __CLIENTWEB_H__

#include "web.h"

#define error(msg) do{ \
	perror(msg); \
	exit(1); \
}while(0)

#define errPrint(msg) do{ \
	fprintf(stderr, msg);	\
	exit(0);	\
}while(0)

#define MAXFILES 64
#define PORT "80"

/* connect 连接的状态 */ 
#define F_CONNECTING 1	/* 连接中 */ 
#define F_READING	 2	/* 已经连接, 正在读 */ 
#define F_DONE		 4	/* 已完成 */ 

#define GET_CMD		"GET %s HTTP/1.0\r\n\r\n"

struct file{
	char *f_name;
	char *f_host;
	int fd;
	int f_flags;
}file[MAXFILES];

int nonblock(int);
void home_page(const char *, const char *);
void start_connect(struct file *);
void write_get_cmd(struct file *);
int tcp_connect(const char *, const char *);
struct addrinfo * host_serv(const char *, const char *, int, int);

/*
 * nconn : 当前创建连接的 TCP 个数
 * nlefttoconn : 没有连接的 TCP 个数
 * nlefttoread : 在准备读取的 TCP 个数
 * maxfd : 打开的最大文件描述符
 */ 
int nconn, nfiles, nlefttoconn, nlefttoread, maxfd;
fd_set rset, wset;

#endif
