/*************************************************************************
  > File Name: inet_network.c
  > Author: Function_Dou
  > Mail: NOT
  > Created Time: 2019年02月02日 星期六 12时12分27秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    in_addr_t addr;

    // 将点分十进制IP转化为主机字节序（二进制位小端存储）
    // 如果失败：返回-1；
    // 如果成功：返回主机字节序对应的数；
    addr = inet_network(argv[1]);
    printf("0x%08x\n", addr);

    exit(EXIT_SUCCESS);
}
