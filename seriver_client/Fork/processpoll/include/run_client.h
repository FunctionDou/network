#ifndef __RUNCLIENT_H__
#define __RUNCLIENT_H__

#include "processpool.h"

enum{
	BUFSIZE = 1024
};

struct fds{
	int epollfd;
	int sockfd;
};

void fdsinit(struct fds *, int, int);

int processing(struct fds *);

#endif
