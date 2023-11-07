#include "compose/compose.h"

#include "kernel/dev/speaker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <widgets/widget.h>
#include <widgets/button.h>

static compose_client_t* client = NULL;
int beeper = 0;

void click(widget* w) {
	if(beeper >= 0) {
		ioctl(beeper, KDMKTONE, (void*) 100000);
	}
}

int main(int argc, char** argv) {
	beeper = open("/dev/pcspkr", O_RDONLY);

	client = compose_cl_connect("/tmp/.compose.sock");
	if(!client) {
		perror("Failed to connect to compose server.");
		return 1;
	}

	widgets_init();

	widget* login = widget_create(client, WIDGET_TYPE_WINDOW, NULL, NULL);

	button* butt = malloc(sizeof(button));
	butt->flags = 0;
	butt->click = click;

	widget_create(client, WIDGET_TYPE_BUTTON, login, butt);

	widget_draw(login);

	while(1) {
		widgets_tick(login);
	}

	return 0;
}
