#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/tty.h>
#include <unistd.h>
#include <kernel/dev/ps2.h>
#include <input/mouse.h>
#include <compose/compose.h>

static const char* startup_app = NULL;
static char** startup_args = NULL;

void parse_args(int argc, char** argv) {
	if(argc < 2) {
		return;
	}
	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "--startup")) {
			if(i + 1 < argc) {
				startup_app = argv[i + 1];
				i++;
			}
		}
		else if(!strcmp(argv[i], "--")) {
			if(i + 1 < argc) {
				startup_args = argv + i + 1;
				break;
			}
		}
	}
}

int main(int argc, char** argv) {	
	parse_args(argc, argv);

	struct termios t;
	tcgetattr(STDIN_FILENO, &t);
	t.c_lflag &= ~ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &t);

	printf("Starting server...\n");

	remove("/tmp/.compose.lock");
	compose_server_t* srv = compose_sv_create("/tmp/.compose.sock");

	if(startup_app) {
		if(!fork()) {
			execve(startup_app, startup_args, NULL);
			exit(-1);
		}
	}

	fb_fill(&srv->framebuffer, 0xFF111111);
	fb_flush(&srv->framebuffer);

	printf("Server started.\n");
	
	while(1) {
		compose_sv_tick(srv);
	}

	return 0;
}
