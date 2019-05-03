#include "processpool.h"

int main(int argc, char *argv[]){
	int listenfd;
	int ret;
	socklen_t len;
	struct sockaddr_in addr;
	struct processpool propool;

	memset(&propool, 0, sizeof(propool));
	addr.sin_port = htons(atoi(argv[1]));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	len = sizeof(addr);

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(listenfd < 0)
		goto exit;
	ret = bind(listenfd, (struct sockaddr *)&addr, len);
	if(ret < 0)
		goto exit;
	ret = listen(listenfd, 5);
	if(ret < 0)
		goto exit;

	propool.listen_fd = listenfd;

	run(&propool);

exit:
	exit(0);
}
