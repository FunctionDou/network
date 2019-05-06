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


int main(int argc, char *argv[]){
	int fdClinet;
	struct sockaddr_in serAddr;
	serAddr.sin_family = AF_INET;
	inet_aton(argv[1], &serAddr.sin_addr);
	serAddr.sin_port = htons(atoi(argv[2]));

	
	fdClinet = socket(AF_INET, SOCK_DGRAM, 0);

	// 使用connect之后就可以使用 read 和 write 进行通信了
	if(connect(fdClinet, (struct sockaddr*)&serAddr, sizeof(serAddr)) == -1)
		error("connect");

	char buf[1024];
	int size;
	while(1){
		size =read(STDIN_FILENO, buf, sizeof(buf));
		if(size < 0)
			break;
		write(fdClinet, buf, strlen(buf));
		size = read(fdClinet, buf, sizeof(buf));
		if(size < 0)
			break;
		write(STDOUT_FILENO, buf, size);
	}
	close(fdClinet);

	return 0;
}
