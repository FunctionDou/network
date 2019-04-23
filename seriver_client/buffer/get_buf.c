#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[]){
    int tcp_buf, udp_buf;
    int sock;
    socklen_t len;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    len = sizeof(sock);
    getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (void *)&tcp_buf, &len);
    printf("tcp buff : %d\n", tcp_buf);

    close(sock);

    sock = socket(PF_INET, SOCK_DGRAM, 0);
    len = sizeof(sock);
    getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (void *)&udp_buf, &len);
    printf("udp buff : %d\n", udp_buf);

    close(sock);


    return 0;
}
