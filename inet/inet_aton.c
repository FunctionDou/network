#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#ifdef __STRUCT_IN_ADDR

struct in_addr
{
	typedef uint32_t in_addr_t;
	in_addr_t s_addr;
};

#endif

int main(int argc, char *argv[])
{
	if(argc != 2)
		exit(-1);

	struct in_addr addr;
	// inet_aton是一个计算机函数，功能是将一个字符串IP地址转换为一个32位的网络序列IP地址。如果这个函数成功，函数的返回值非零，如果输入地址不正确则会返回零
	if(inet_aton(argv[1], &addr) == 0)
		exit(-1);
	printf("0x%08x\n", addr.s_addr);
	printf("%d\n", inet_aton(argv[1], &addr));
 
    exit(EXIT_SUCCESS);
}
