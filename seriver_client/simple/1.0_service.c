/*************************************************************************
  > File Name: service.c
  > Author: Function_Dou
  > Mail: NOT
  > Created Time: 2019年02月07日 星期四 12时17分23秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define EXIT(msg) do{	\
   perror(msg); \
   exit(-1); \
}while(0)

int main(int argc, char *argv[])
{
   int sockfd;
   struct sockaddr_in serviced_addr;
   serviced_addr.sin_family = AF_INET;
   serviced_addr.sin_addr.s_addr = inet_addr("192.168.1.16");
   serviced_addr.sin_port = htons(8080);

   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if(sockfd < 0)
      EXIT("socket");

   socklen_t service_len = sizeof(serviced_addr);
   int ret;
   ret = bind(sockfd, (struct sockaddr *)&serviced_addr, service_len);
   if(ret < 0)
      EXIT("bind");

   ret = listen(sockfd, 5);
   if(ret < 0)
      EXIT("listen");


   while(1)
      pause();

   exit(EXIT_SUCCESS);
}
