#include "sys/select.h"
#include "sys/un.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/tty.h>
#include <unistd.h>

int main(int argc, char** argv) {	
	struct termios t;
	tcgetattr(STDIN_FILENO, &t);
	t.c_lflag &= ~ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &t);

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

	int kbdfd = open("/dev/kbd", O_RDONLY);
	int mosfd = open("/dev/mouse", O_RDONLY);

	if(kbdfd < 0 || mosfd < 0) {
		return 3;
	}

	int maxfd = kbdfd > mosfd ? kbdfd : mosfd; 

	fd_set rset;
	FD_ZERO(&rset);

	printf("Waiting for events...\n");

	while(1) {
		FD_ZERO(&rset);
		FD_SET(kbdfd, &rset);
		FD_SET(mosfd, &rset);

		int d = select(maxfd + 1, &rset, NULL, NULL, NULL);

		printf("select(): %d\n", d);
		
		uint8_t a;

		if(FD_ISSET(kbdfd, &rset)) {
			printf("Keyboard event received!\n");
			read(kbdfd, &a, 1);
		}

		if(FD_ISSET(mosfd, &rset)) {
			printf("Mouse event received!\n");
			read(mosfd, &a, 3);
		}
	}

	return 0;
}
