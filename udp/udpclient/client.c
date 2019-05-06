#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#define error(s) {	\
	perror(s);	\
	exit(1);	\
}

// 输入 : IP地址 端口号
int main(int argc, char *argv[]){
	int fdClinet;
	struct sockaddr_in serAddr, cliAddr;
	serAddr.sin_family = AF_INET;
	inet_aton(argv[1], &serAddr.sin_addr);
	serAddr.sin_port = htons(atoi(argv[2]));

	
	fdClinet = socket(AF_INET, SOCK_DGRAM, 0);

	char buf[1024];
	int size;
	socklen_t clientlen;
	while(1){
		size = read(STDIN_FILENO, buf, sizeof(buf));
		if(size <= 0)
			break;
		// 向指定端口发送数据
		sendto(fdClinet, buf, size, 0, (struct sockaddr *)&serAddr, sizeof(serAddr));
		clientlen = sizeof(cliAddr);
		// 接收并获取对端的UDP套接字信息
		size = recvfrom(fdClinet, buf, sizeof(buf), 0, (struct sockaddr *)&cliAddr, &clientlen);
		if(size < 0)
			break;
		write(STDOUT_FILENO, buf, size);
	}
	close(fdClinet);

	return 0;
}
