#include "client_web.h"

// 将文件描述符设置为非阻塞
int nonblock(int fd){
	int oldfd;

	oldfd = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, oldfd | O_NONBLOCK);

	return oldfd;
}

// 向目标服务写入数据后, 设置IO复用用于监听描述符可读
void write_get_cmd(struct file *fptr){
	int n;
	char line[MAXFILES];

	n = snprintf(line, sizeof(line), GET_CMD, fptr->f_name);
	write(fptr->fd, line, n);
	printf("write %d bytes for %s\n", n, fptr->f_name);

	fptr->f_flags = F_READING;

	FD_SET(fptr->fd, &rset);
	if(fptr->fd > maxfd)
		maxfd = fptr->fd;
}

// 创建一个 TCP 连接, 用于获取主页
void home_page(const char * host, const char * fname){
	int fd, n;
	char line[MAXFILES];

	fd = tcp_connect(host, PORT);
	n = snprintf(line, sizeof(line), GET_CMD, fname);
	write(fd, line, n);

	while(1){
		if((n = read(fd, line, sizeof(line))) == 0)
			break;
		printf("read %d bytes of home page\n", n);
	}
	printf("end-of-file on home page\n");
	close(fd);
}

// 获取目标主机信息后设置为非阻塞再与之建立连接
void start_connect(struct file *fptr){
	int	fd, flags, n;
	struct addrinfo *ai;

	ai = host_serv(fptr->f_host, PORT, 0, SOCK_STREAM);

	fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	fptr->fd  = fd;
	flags = nonblock(fd);

	if((n = connect(fd, ai->ai_addr, ai->ai_addrlen)) < 0){
		if(errno != EINPROGRESS)
			errPrint("nonblock connect error");
		fptr->f_flags = F_CONNECTING;
		FD_SET(fd, &rset);
		FD_SET(fd, &wset);
		if(fd > maxfd)
			maxfd = fd;
	}
	else if(n >= 0)
		write_get_cmd(fptr);
}
