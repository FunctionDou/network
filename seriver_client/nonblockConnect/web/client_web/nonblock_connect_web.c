#include "client_web.h"
#include <time.h>



int main(int argc, char *argv[]){
	int i, fd, maxnconn, flags, err;
	socklen_t n;
	char buf[MAXFILES];
	fd_set rs, ws;
	clock_t home_start, home_end, connectnum_start, connectnum_end;


	if(argc < 5)
		errPrint("usags: web <#conns> <hostname> <homepage> <file> ...");
	maxnconn = atoi(argv[1]);

	// 获取需要连接的网页
	nfiles = argc - 4 > MAXFILES ? MAXFILES : argc - 4;
	for(int i = 0; i < nfiles; ++i){
		file[i].f_name = argv[i+4];
		file[i].f_host = argv[2];
		file[i].f_flags = 0;
	}
	printf("files = %d\n", nfiles);

	home_start = clock();
	home_page(argv[2], argv[3]);	/* 与主页先保持连接 */ 
	home_end = clock();

	FD_ZERO(&rset);
	FD_ZERO(&wset);
	maxfd = -1;
	nlefttoread = nlefttoconn = nfiles;
	nconn = 0;

	connectnum_start = clock();
	while(nlefttoread > 0){
		/* 同时能建立的连接个数 */ 
		while(nconn < maxnconn && nlefttoconn > 0){
			for(i = 0; i < nfiles; ++i)
				if(file[i].f_flags == 0)
					break;
			if(i == nfiles)
				errPrint("nlefttoconn but nothing found");
			start_connect(&file[i]);
			nconn++;
			nlefttoconn--;
		}

		rs = rset;
		ws = wset;
		n = select(maxfd + 1, &rs, &ws, NULL, NULL);

		// 向服务端发送要获取的信息, 然后关闭写监听后在来监听读
		for(i = 0; i < nfiles; ++i){
			flags = file[i].f_flags;
			if(flags == 0 || (flags & F_DONE))
				continue;

			fd = file[i].fd;
			if(flags & F_CONNECTING && (FD_ISSET(fd, &rs) || FD_ISSET(fd, &ws))){
				n = sizeof(err);
				if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &n) < 0 || err != 0){
					error("connect error");
				}
				/* 写完后, 关闭写监听, 打开读监听 */ 
				printf("connect established for %s\n", file[i].f_name);
				FD_CLR(fd, &wset);
				write_get_cmd(&file[i]);
			}
			else if(flags & F_READING && FD_ISSET(fd, &rs)){
				if((n = read(fd, buf, sizeof(buf))) == 0){
					printf("end-of-file on %s\n", file[i].f_name);
					close(fd);
					file[i].f_flags = F_DONE;
					FD_CLR(fd, &rset);
					nconn--;	// 可以建立下一个连接
					nlefttoread--;
				}
				else
					printf("read %d bytes from %s\n", n, file[i].f_name);
			}
		}
	}
	connectnum_end = clock();

	printf("\n\nhome connet time :  %f\n", (double)(home_end - home_start)/CLOCKS_PER_SEC);
	printf("num connect time : %f\n", (double)(connectnum_end - connectnum_start)/CLOCKS_PER_SEC);
	printf("sum time : %f\n", (double)(connectnum_end - home_start)/CLOCKS_PER_SEC);

	return 0;
}
