#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#define EXIT(msg) do{	\
   perror(msg); \
   exit(-1); \
}while(0)

int main(int argc, char *argv[])
{
   int sockfd;
   struct sockaddr_in serviced_addr;
   serviced_addr.sin_family = AF_INET;
   serviced_addr.sin_addr.s_addr = inet_addr("192.168.1.16");	// 为本机IP地址
   serviced_addr.sin_port = htons(8080);

   // 创建套接字
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if(sockfd < 0)
      EXIT("socket");

   socklen_t service_len = sizeof(serviced_addr);
   int ret;
   // 服务端端口绑定
   ret = bind(sockfd, (struct sockaddr *)&serviced_addr, service_len);
   if(ret < 0)
   {
      if(errno == EADDRINUSE)
	 fprintf(stderr, "addr %s error\n", inet_ntoa(serviced_addr.sin_addr));
      else if(errno == EINVAL)
	 fprintf(stderr, "port %d error\n", ntohs(serviced_addr.sin_port));
      EXIT("bind");
   }
   // 服务端开始监听
   ret = listen(sockfd, 5);
   if(ret < 0)
      EXIT("listen");

   int clientfd;
   struct sockaddr_in clientAddr;
   socklen_t clientLen;
   while(1)
      ;
   // 接收连接
   if((clientfd = accept(sockfd, (struct sockaddr *)&clientAddr, &clientLen)) < 0)
      EXIT("accept");

   char buf[1024];
   int n;
   // 回射
   while(1)
   {
      n = recv(clientfd, buf, sizeof(buf), 0);
      if(n == 0)
	 break;
      send(clientfd, buf, n, 0);
   }


   exit(EXIT_SUCCESS);
}
