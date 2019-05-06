#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#define error(s) {\
	perror(s); \
	exit(1); \
}

// 输入 : 端口号
int main(int argc, char *argv[]){
	int fdServer;
	struct sockaddr_in udpAddr, cliAddr;
	memset(&udpAddr, 0, sizeof(udpAddr));
	udpAddr.sin_family = AF_INET;
	udpAddr.sin_addr.s_addr = htonl(INADDR_ANY);	// 使用本机地址
	udpAddr.sin_port = htons(atoi(argv[1]));
	
	fdServer = socket(AF_INET, SOCK_DGRAM, 0);
	if(fdServer == -1)
		error("socket");
	if(bind(fdServer, (struct sockaddr *)&udpAddr, sizeof(udpAddr)) == -1)
		error("bind");


	size_t size;
	char buf[1024];
	while(1){
		socklen_t socklen = sizeof(udpAddr);
		// 接收并获取对端的UDP套接字信息
		size = recvfrom(fdServer, buf, sizeof(buf), 0, (struct sockaddr *)&cliAddr, &socklen);
		// 通过获取到的UDP套接字将数据回射回去
		sendto(fdServer, buf, size, 0, (struct sockaddr *)&cliAddr, socklen);
	}
	close(fdServer);

	return 0;
}
