#include "sys/un.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char** argv) {

	int sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if(sockfd < 0) {
		return 1;
	}

	struct sockaddr_un addr;
	addr.sun_family = AF_LOCAL;
	strcpy(addr.sun_path, "/tmp/.compositor.sock");

	if(bind(sockfd, &addr, sizeof(addr)) < 0) {
		return 2;
	} 

	listen(sockfd, 10);

	while(1) {
		int connectfd = accept(sockfd, NULL, NULL);

		write(connectfd, "HELLO!", 7);
	}

	return 0;
}
