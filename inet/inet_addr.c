/*************************************************************************
  > File Name: inet_addr.c
  > Author: Function_Dou
  > Mail: NOT
  > Created Time: 2019年02月02日 星期六 12时07分36秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main(int argc, char *argv[])
{
    in_addr_t addr;

    // inet_addr()的功能是将一个点分十进制的IP转换成一个长整数型数（u_long类型）
    // 如果失败：返回INADDR_NONE；
    // 如果成功：返回IP对应的网络字节序的数；
    addr = inet_addr(argv[1]);
    printf("0x%08x\n", addr);

    exit(EXIT_SUCCESS);
}
