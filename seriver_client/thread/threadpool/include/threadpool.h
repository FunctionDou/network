#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include "list.h"
#include "httpd.h"
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>

enum {
	MAX_THREAD_NUM = 10,
	MAX_USER_NUM = 65536
};

struct threadpool{
	pthread_t threads[MAX_THREAD_NUM];	/* 线程池 */ 
	SLIST_HEAD(Head, httpd) head;		/* 请求队列 */
	struct httpd *last;				/* 请求队列的尾 */ 
	int count;							/* 请求队列中的个数 */ 

	sem_t sem;							/* 处理任务 */ 
	pthread_mutex_t lock;				/* 互斥锁 */ 
	int stop;							/* 结束线程 */ 
};

void initpool(struct threadpool *);
int append(struct threadpool *, struct httpd* );

#endif
