#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/tty.h>
#include <unistd.h>
#include <kernel/dev/ps2.h>
#include <input/mouse.h>
#include <compose/compose.h>

int main(int argc, char** argv) {	
	struct termios t;
	tcgetattr(STDIN_FILENO, &t);
	t.c_lflag &= ~ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &t);

	compose_server_t* srv = compose_server_create("/tmp/.compose.sock");

	fb_fill(&srv->framebuffer, 0xFF000000);

	while(1) {
		compose_server_tick(srv);
	}

	return 0;
}
