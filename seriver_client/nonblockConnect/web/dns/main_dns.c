#include "dns.h"

int main(int argc, char *argv[]){
	char *p = NULL;
	while(--argc){
		p = *++argv;
		dns(p);
	}

	exit(0);
}
