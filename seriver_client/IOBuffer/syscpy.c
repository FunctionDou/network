#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define BUF_SIZE 50

int main(int argc, char *argv[]){
    int readfd, writefd;
    int len;
    char buf[BUF_SIZE];
    clock_t start, end;

    readfd = open(argv[1], O_RDONLY);
    writefd = open("txt", O_WRONLY | O_CREAT, 0755);

    start = clock();
    while((len = read(readfd, buf, sizeof(buf))) > 0)
	write(writefd, buf, len);

    end = clock();
    printf("%f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    close(readfd);
    close(writefd); 


    return 0;
}
