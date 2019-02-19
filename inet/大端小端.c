#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

union tmp
{
    int i;
    char ch;
};

int main(int argc, char *argv[])
{
	int i = 0x12345678;
	char ch = *(char *)&i;

	union tmp t;
	t.i = 0x12345678;


	uint32_t l = 0x12345678;
	uint32_t ll = htonl(l);
	printf("ll = %x, l = %x\n", ll, l);
	if(ch == 0x78 && t.ch == 0x78)
		printf("小端\n");
	else
		printf("大端\n");
 
    exit(EXIT_SUCCESS);
}
