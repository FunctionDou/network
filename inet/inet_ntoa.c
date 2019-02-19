#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>


int main(int argc, char *argv[])
{
	struct in_addr net_addr;
	net_addr.s_addr = 0x0100007f;
	char *host_addr;

	// 将一个32位网络字节序的二进制IP地址转换成相应的点分十进制的IP地址
	host_addr = inet_ntoa(net_addr);
	printf("%s\n", host_addr);
 
    exit(EXIT_SUCCESS);
}
