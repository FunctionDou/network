#include "httpd.h"

// http 请求方式
enum{
	GET = 0,
	POST, HEAD, PUT, DELETE, CONNECT, OPTIONS, TRACE,
};

// 解析客户请求, 状态机的状态
enum{
	CHECK_REQUESTLINE = 0,
	CHECK_HEADER,
	CHECK_CONTENT
};

// HTTP 应答状态
enum{
	GET_REQUEST = 0,	/* 获取到一个完整的客户请求 */ 
	BAD_REQUEST,		/* 客户请求有语法错误 */ 
	NO_REQUSET,			/* 请求的数据不完整, 需要继续请求 */ 
	CLOSE_CONNECTION,	/* 客户已经关闭 */ 
	INTERNAL_ERROR,		/* 服务器内部错误 */ 
	FORBIDDEN_REQUEST,
	NO_RESOURCE,		/* 没有找到文件 */ 
	FILE_REQUSET,		/* 正确的映射了文件 */ 
};

// 请求行的状态
enum{
	LINK_OK = 0,	/* 读取到一个完整的行 */ 
	LINK_BAD,		/* 读取的行有问题 */ 
	LINK_OPEN,		/* 读取的行并不完整 */ 
};

// content-type 状态 
enum{
	CLOSE = 0,
	KEEP_ALIVE,
};

static int setnonblock(int );
static void del_event(int , int );
static void mod_event(int , int , int );

static void init(struct httpd *);

static int parse_request_line(struct httpd *, char *);
static int parse_head(struct httpd *, char *);
static int parse_content(struct httpd *, char *);
static char * get_line(struct httpd *);
static int parse_line(struct httpd *);
static int process_reading(struct httpd *);
static int request_file(struct httpd *);
static void unmap(struct httpd *);

static int to_write(struct httpd *, const char *, ...);
static int process_write(struct httpd *, int );

// 设置文件描述符为非阻塞
int setnonblock(int fd){
	int old_fd;

	old_fd = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, old_fd | O_NONBLOCK);

	return old_fd;
}

// 将文件描述符注册到 epoll 中
void add_event(int epollfd, int fd, int isoneshot){
	struct epoll_event event;
	
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLRDHUP;
	// 设置描述符是只触发一次
	if(isoneshot)
		event.events |= EPOLLONESHOT;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	setnonblock(fd);
}

// 从 epoll 中删除注册的文件描述符
void del_event(int epollfd, int fd){
	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
	close(fd);
}

// 将文件描述符重新注册到 epoll 监听中
void mod_event(int epollfd, int fd, int ev){
	struct epoll_event event;

	event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
	event.data.fd = fd;
	epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

// 初始化 httpd 结构
void inithttpd(struct httpd * http, const int fd, const struct sockaddr * addr){
	http->sockfd = fd;
	memcpy(&http->addr, addr, sizeof(http->addr));	// 深拷贝
	// 将事件注册监听
	add_event(epollfd, fd, 1);

	/*
	int err = 0;
	socklen_t len = sizeof(err);
	if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) != 0)
		fprintf(stderr, "getsockopt : sock error\n");
	int reuse = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	*/ 

	init(http);
}
// 初始化 httpd 的其他参数
void init(struct httpd *http){
	http->readIdx = http->checkIdx = http->startLine = http->writeIdx = 0;
	http->keep = CLOSE;
	http->check_state = CHECK_REQUESTLINE;
	http->method = GET;
}

// 从 内核缓冲区中读出客户端发送的数据并存放到 httpd 中的读缓冲区中以便后续进行分析
int readn(struct httpd *http){
	int n = 0;
	int sum = 0;

	do{
		if(http->readIdx >= READ_BUFSIZE)
			return -1;

		n = recv(http->sockfd, http->readbuf + http->readIdx, sizeof(http->readbuf) - http->readIdx, MSG_WAITALL);
		if(n == -1){
			if(errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			return -1;
		}
		else if(n == 0)
			return 0;

		http->readIdx += n;
		sum += n;
	}while(1);

	return sum;
}

// 关闭连接并注销监听的事件
void close_conn(struct httpd *http){
	if(http->sockfd != -1){
		del_event(epollfd, http->sockfd);
		http->sockfd = -1;
	}
}



/*************** 读取缓冲区数据过程 *************/ 

// 缓冲区中一行数据的位置
char *get_line(struct httpd *http){
	return http->readbuf + http->startLine;
}

// 读取一个完整的行
int parse_line(struct httpd *http){
	char c;
	char *cli = http->readbuf;

	for(; http->checkIdx < http->readIdx; ++http->checkIdx){
		c = cli[http->checkIdx];

		if(c == '\r'){
			// \r 是缓冲区中最后一个数据, 则还需要继续读取
			if((http->checkIdx + 1) == http->readIdx)
				return LINK_OPEN;
			// \r\n 一个完整的行数据
			else if(cli[http->checkIdx + 1] == '\n'){
				cli[http->checkIdx++] = '\0';
				cli[http->checkIdx++] = '\0';
				
				return LINK_OK;
			}
			return LINK_BAD;
		}
		// 重新新读取的数据
		else if(c == '\n'){
			if(http->checkIdx > 1 && (cli[http->checkIdx - 1] == '\r')){
				cli[http->checkIdx-1] = '\0';
				cli[http->checkIdx++] = '\0';

				return LINK_OK;
			}
		}
	}
	
	return LINK_BAD;
}



/******************** 写缓冲区写入数据以及发送给客户端 *************/ 

// 发送数据给客户端
int writen(struct httpd *http){
	int tmp = 0;
	int have_data = 0;
	int sockfd;
	int send_data = http->writeIdx;

	// 写缓冲区中没有数据, 则清除 http结构 中的状态监听读事件
	if(send_data == 0){
		mod_event(epollfd, http->sockfd, EPOLLIN);
		init(http);
		return 0;
	}

	sockfd = http->sockfd;
	while(1){
		tmp = writev(sockfd, http->iv, http->iv_conut);
		// tmp = write(sockfd, http->writebuf, strlen(http->writebuf));
		if(tmp <= -1){
			if(errno == EAGAIN){
				mod_event(epollfd, sockfd, EPOLLOUT);
				return 0;
			}
			unmap(http);
			return -1;
		}

		send_data -= tmp;
		have_data += tmp;
		if(send_data <= have_data){
			unmap(http);
			mod_event(epollfd, sockfd, EPOLLIN);
			if(http->keep == KEEP_ALIVE){
				// +++++
				return 0;
			}
			else 
				return -1;
		}
	}

}

// 往写缓冲区中写入数据
int to_write(struct httpd *http, const char *format, ...){
	if(http->writeIdx >= WRITE_BUFSIZE)
		return -1;

	int len;
	va_list arg_list;

	va_start(arg_list, format);
	len = vsnprintf(http->writebuf + http->writeIdx, WRITE_BUFSIZE - http->writeIdx, format, arg_list);
	if(len >= (WRITE_BUFSIZE - 1 - http->writeIdx))
		return -1;

	http->writeIdx += len;
	va_end(arg_list);

	return 0;
}


/************* 处理请求, 设置状态机状态 ************/ 

// 解析 http 请求行 : GET http://www.baidu.com/index.html http/1.1
int parse_request_line(struct httpd *http, char *cli){
	char *url = NULL;
	char *version = NULL;
	
	url = strpbrk(cli, " ");	// 在 cli 中找出第一个与 " " 匹配的位置
	if(!url)
		return BAD_REQUEST;
	*url++ = '\0';	// 将字符串分割

	if(strcasecmp(cli, "GET") == 0)
		http->method = GET;
	else if(strcasecmp(cli, "POST") == 0)
		http->method = POST;
	else
		return BAD_REQUEST;
	
	url += strspn(url, " ");	// 跳过空白字符 
	version = strpbrk(url, " "); // 定位下一个空白字符的位置
	if(!version)
		return BAD_REQUEST;
	*version++ = '\0';	// 将 url 切割
	version += strspn(version, " ");	// 跳过所有空白字符
	if(strncasecmp(version, "HTTP/", 5) != 0)
		return BAD_REQUEST;
	if(strncasecmp(url, "http://", 7) == 0)	{
		url += 7;
		url = strchr(url, '/');	// 定位 第一个文件 / 的位置
	}
	
	if(!url || url[0] != '/')
		return BAD_REQUEST;

	http->url = url;
	http->version = version;
	http->check_state = CHECK_CONTENT;	// 修改有限状态机, 准备执行解析下一部分

	return NO_REQUSET;
}


// 解析请求头部信息
int parse_head(struct httpd *http, char *cli){
	// 请求解析完毕
	if(cli[0] == '\0'){
		// 请求的方式是 HEAD 
		if(http->method == HEAD)
			return GET_REQUEST;
		if(http->content_len != 0){
			http->check_state = CHECK_CONTENT;	// 修改有限状态机, 准备执行解析下一部分
			return NO_REQUSET;
		}
		return GET_REQUEST;
	}
	// 主机名
	else if(strncasecmp(cli, "Host:", 5) == 0){
		cli += 5;
		cli += strspn(cli, " ");
		http->host = cli;
	}
	// 连接状态. keep-alive 是http 1.1 才有. 默认为 CLOSE 状态
	else if(strncasecmp(cli, "Connection:", 11) == 0){
		cli += 11;
		cli += strspn(cli," ");
		if(strcasecmp(cli, "Keep-Alive") == 0)
			http->keep = KEEP_ALIVE;
	}
	// 请求主体的字节数
	else if(strncasecmp(cli, "Content-Length:", 15) == 0){
		cli += 15;
		cli += strspn(cli, " ");
		http->content_len = atoi(cli);
	}
	// 暂时无法解析的部分
	else
		fprintf(stderr, "unkown head %s\n", cli);

	return NO_REQUSET;
}


// 解析消息体 (这里并没有完整的解析)
int parse_content(struct httpd *http, char *cli){
	if(http->readIdx >= (http->content_len + http->checkIdx)){
		cli[http->content_len] = '\0';
		return GET_REQUEST;
	}
	return NO_REQUSET;
}



/************* 有限状态机 *******************/ 

// 有限状态机处理过程
int process_reading(struct httpd *http){
	int line_status = LINK_OK;
	int ret = NO_REQUSET;
	char *cli = NULL;

	// 从缓冲区中读取一行数据, 并设置新的缓冲区头
	while(((http->check_state == CHECK_CONTENT) && (line_status == LINK_OK)) || 
			((line_status = parse_line(http))) == LINK_OK){
		cli = get_line(http);
		http->startLine = http->checkIdx;

		printf("get :  %s\n", cli);

		// ret 返回的是 NO_REQUSET 则继续处理
		switch(http->check_state){
			// 处理请求行
			case CHECK_REQUESTLINE:
				ret = parse_request_line(http, cli);
				if(ret == BAD_REQUEST)
					return BAD_REQUEST;
				break;
			// 处理头部信息
			case CHECK_HEADER:
				ret = parse_head(http, cli);
				if(ret == GET_REQUEST)	// 获取到一个完整的客户请求(HEAD)
					return request_file(http);
				break;
			// 处理主体信息
			case CHECK_CONTENT:
				ret = parse_content(http, cli);
				if(ret == GET_REQUEST)	// 获取到一个完整的客户请求
					return request_file(http);
				line_status = LINK_OPEN;
				break;
			default:
				return INTERNAL_ERROR;
		}
	}
	return NO_REQUSET;
}


/******************** 请求文件的处理 *******************/ 

const char *root = "htdocs";
// 请求文件位置, 并将文件进行映射
int request_file(struct httpd *http){
	int fd;

	// 文件地址(可能是目录)
	snprintf(http->filename, FILENAME_LEN, "%s%s", root, http->url);;

	// 如果是目录, 则添加 index.html 
	if(http->filename[strlen(http->filename) - 1] == '/')
		strcat(http->filename, "index.html");

	// 检查如果文件不存在
	if(stat(http->filename, &http->file_stat) == -1)
		return NO_RESOURCE;

	/*
	// 如果是目录, 则添加 index.html 
	if(http->file_stat.st_mode & S_IFDIR)
		strcat(http->filename, "index.html");
	*/


	// 如果没有读权限
	if(!(http->file_stat.st_mode & S_IROTH))
		return FORBIDDEN_REQUEST;
	// 如果是目录. 当然如果是目录也可以在文件名后加上 index.html
	if(S_ISDIR(http->file_stat.st_mode))
		return BAD_REQUEST;

	// 因为暂时只是访问静态文件, 所以只读方式即可
	fd = open(http->filename, O_RDONLY);
	http->mm_addr = (char *)mmap(0, http->file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

	close (fd);

	return FILE_REQUSET;
}

// 解除文件映射
void unmap(struct httpd *http){
	if(http->mm_addr){
		munmap(http->mm_addr, http->file_stat.st_size);
		http->mm_addr = 0;
	}
}






int add_response(struct httpd *http, const char *format, ...){
	if(http->writeIdx >= WRITE_BUFSIZE)
		return 0;
	va_list arg;
	va_start(arg, format);
	int len;
	len = vsnprintf(http->writebuf + http->writeIdx, WRITE_BUFSIZE - 1 - http->writeIdx, format, arg);
	if(len >= (WRITE_BUFSIZE - 1 - http->writeIdx))
		return 0;
	http->writeIdx += len;
	va_end(arg);

	return 1;
}

void add_head(struct httpd *http, int len){
	add_response(http, "Content-Length: %d\r\n", len);
	add_response(http, "Connection: %s\r\n", (http->keep == CLOSE) ? "close" : "keep-alive");
	add_response(http, "%s", "\r\n");
}

void add_content(struct httpd *http, const char *content){
	add_response(http, "%s", content);
}

int add_status_line(struct httpd *http, int status, const char *title){
	return add_response(http, "%s %d %s\r\n", "HTTP/1.1", status, title);
}

const char *err_400 = "Bad Request";
const char *err_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";

// 回复客户端状态
int process_write(struct httpd *http, int ret){
	switch(ret){
		case INTERNAL_ERROR:
			break;
		case BAD_REQUEST:
			// bad_request(http);
			add_status_line(http, 400, err_400);
			add_head(http, strlen(err_400_form));
			add_content(http, err_400_form);
			printf("\n\nbad\n");
			break;
		case NO_RESOURCE:
			break;
		case FORBIDDEN_REQUEST:
			break;
		case FILE_REQUSET:
			if(http->file_stat.st_size != 0){
				http->iv[0].iov_base = http->writebuf;
				http->iv[0].iov_len = http->writeIdx;
				http->iv[1].iov_base = http->mm_addr;
				http->iv[1].iov_len = http->file_stat.st_size;
				http->iv_conut = 2;
				return 0;
			}
			else{
				return -1;
			}
		default:
			return -1;
	}
	http->iv[0].iov_base = http->writebuf;
	http->iv[0].iov_len = http->writeIdx;
	http->iv_conut = 1;

	return 0;
}

void processing(struct httpd *http){
	int ret, writeret;
	int sockfd;
	
	sockfd = http->sockfd;
	ret = process_reading(http);
	if(ret == NO_REQUSET){
		mod_event(epollfd, sockfd, EPOLLIN);
		return ;
	}

	writeret = process_write(http, ret);
	if(writeret != 0)
		close_conn(http);
	printf("error\n");
	mod_event(epollfd, sockfd, EPOLLOUT);
	// writen(http);
}
