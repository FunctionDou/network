#include "threadpool.h"

void *worker(void *);
void run(struct threadpool *);

void initpool(struct threadpool *thrd){
	thrd->stop = 0;
	thrd->last = NULL;
	thrd->count = 0;
	SLIST_INIT(&thrd->head);
	pthread_mutex_init(&thrd->lock, NULL);
	sem_init(&thrd->sem, 0, 0);

	// 创建线程池
	for(int i = 0; i < MAX_THREAD_NUM; ++i){
		if(pthread_create(thrd->threads + i, NULL, worker, (void *)thrd) != 0){
			fprintf(stderr, "pthread_create error\n");
			return;
		}
	}
}

int append(struct threadpool * thrd, struct httpd *request){
	pthread_mutex_lock(&thrd->lock);
	/* 队列是否超过了设置的长度 */
	if(thrd->count > MAX_USER_NUM){
		pthread_mutex_unlock(&thrd->lock);
		return -1;
	}
	/* 请求加入到请求队列尾 */ 
	if(SLIST_EMPTY(&thrd->head)){
		SLIST_INSERT_HEAD(&thrd->head, request, next);
		SLIST_FIRST(&thrd->head) = request;
		thrd->last = request;
	}
	else{
		SLIST_INSERT_AFTER(thrd->last, request, next);
		thrd->last = request;
	}
	++thrd->count;

	pthread_mutex_unlock(&thrd->lock);
	sem_post(&thrd->sem);
	
	return 0;
}

void *worker(void *arg){
	pthread_detach(pthread_self());
	printf("worker pthread = %ld\n", pthread_self());
	struct threadpool *thrd = (struct threadpool *)arg;

	run(thrd);

	return thrd;
}

void run(struct threadpool *thrd){
	while(!thrd->stop){
		sem_wait(&thrd->sem);
		pthread_mutex_lock(&thrd->lock);
		if(SLIST_EMPTY(&thrd->head)) { /* 请求队列为空 */ 
			printf("pthead %ld   head NULL\n", pthread_self());
			pthread_mutex_unlock(&thrd->lock);
			continue;
		}
		/* 获取请求队列头并删除 */ 
		struct httpd *request = SLIST_FIRST(&thrd->head);
		SLIST_REMOVE_HEAD(&thrd->head, next);
		pthread_mutex_unlock(&thrd->lock);

		if(!request)
			continue;
		processing(request);
	}
}
