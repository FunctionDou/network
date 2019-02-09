/*************************************************************************
  > File Name: client.c
  > Author: Function_Dou
  > Mail: NOT
  > Created Time: 2019年02月07日 星期四 12时22分41秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define EXIT(msg) do {\
    perror(msg);	\
    exit(-1);	\
}while(0)

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in service_addr, client_addr;

    service_addr.sin_port = htons(8080);
    service_addr.sin_addr.s_addr = inet_addr("196.168.1.16");
    service_addr.sin_family = AF_INET;

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	EXIT("socket");

    if(connect(sockfd, (struct sockaddr *)&service_addr, sizeof(service_addr)) != 0)
	EXIT("connect");

    socklen_t cli_len = sizeof(client_addr);
    getsockname(sockfd, (struct sockaddr *)&client_addr, &cli_len);
    printf("%s %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    exit(EXIT_SUCCESS);
}
