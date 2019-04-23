#include <stdio.h>
#include <unistd.h>
#include <time.h>

#define BUF_SIZE 50

int main(int argc, char *argv[]){
    FILE *readfp, *writefp;
    char buf[BUF_SIZE];
    clock_t start, end;

    readfp = fopen(argv[1], "r");
    writefp = fopen("txt", "w+");

    start = clock();

    while(fgets(buf, sizeof(buf), readfp) != NULL)
	fputs(buf, writefp);

    end = clock();
    printf("%f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);

    fclose(readfp);
    fclose(writefp);

    return 0;
}
