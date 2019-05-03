#include "dns.h"

// 输出指定域名的基本信息
void dns(const char *ptr){
	struct hostent * hptr;
	if((hptr = gethostbyname(ptr)) == NULL){
		fprintf(stderr, "gethostbyname erro for host : %s %s", ptr, hstrerror(h_errno));
		return;
	}

	char **p = NULL;
	char str[INET_ADDRSTRLEN];
	printf("official hostname : %s\n", hptr->h_name);

	for(p = hptr->h_aliases; *p != NULL; ++p){
		fprintf(stderr, "\talias : %s\n", *p);
	}

	switch(hptr->h_addrtype){
		case AF_INET:
			p = hptr->h_addr_list;
			for( ; *p != NULL; ++p)
				printf("\taddress: %s\n", inet_ntop(AF_INET, *p, str, sizeof(str)));
			break;
		default:
			fprintf(stderr, "unkown address type");
			break;
	}
}
