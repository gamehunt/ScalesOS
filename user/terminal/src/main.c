#include "compose/compose.h"

#include "widgets/widget.h"
#include "widgets/win.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/tty.h>

static compose_client_t* client = NULL;
static widget* window    = NULL;

int master, slave;

char buff[10] = {0};

void draw_terminal(widget* window) {
	window_draw(window);
	fb_string(&window->ctx->fb, 0, 0, buff, NULL, 0x0, 0xFFFFFFFF);
}

int main(int argc, char** argv) {
	client = compose_cl_connect("/tmp/.compose.sock");
	if(!client) {
		perror("Failed to connect to compose server.");
		return 1;
	}

	widgets_init();

	widget_properties props = {0};
	props.pos.x = 500;
	props.pos.y = 300;
	props.size.w = 400;
	props.size.h = 200;
	props.padding.left  = 5;
	props.padding.right = 5;
	window = widget_create(client, WIDGET_TYPE_WINDOW, NULL, props, NULL);
	window->ops.draw = draw_terminal;

	int r = openpty(&master, &slave, NULL, NULL, NULL);
	if(r < 0) {
		perror("Failed to open pty");
		return 1;
	}
	
	pid_t child = fork();
	if(!child) {
		dup2(slave, STDIN_FILENO);
		dup2(slave, STDOUT_FILENO);
		dup2(slave, STDERR_FILENO);
		execve("/bin/scsh", NULL, NULL);
		exit(1);
	}

	read(master, buff, 10);

	widget_draw(window);

	while(1) {
		widgets_tick(window);
	}

	return 0;

}
