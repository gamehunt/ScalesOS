#include "compose/compose.h"

#include "kernel/dev/speaker.h"
#include "tga.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <widgets/widget.h>
#include <widgets/button.h>
#include <widgets/input.h>

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

	tga_t* tga = tga_open("/res/test.tga");

	if(tga) {
		int node = shm_open("__lockscreen_bg", O_RDWR | O_CREAT, 0);
		if(node >= 0) {
			size_t sz = tga->w * tga->h * 4;
			ftruncate(node, sz);
			void* mem = mmap(NULL, sz, PROT_WRITE, MAP_SHARED, node, 0);
			memcpy(mem, tga->data, sz);
			msync(mem, sz, MS_SYNC);
			compose_cl_bitmap(client, client->root, 0, 0, tga->w, tga->h, "__lockscreen_bg");
		}
		tga_close(tga);
	}

	widgets_init();

	widget_properties props;
	props.pos.x = 500;
	props.pos.y = 300;
	props.size.w = 200;
	props.size.h = 125;
	widget* login = widget_create(client, WIDGET_TYPE_WINDOW, NULL, props, NULL);

	input* inp = calloc(1, sizeof(input));
	inp->confirm = click;
	inp->placeholder = "Login";
	props.pos.x = 25;
	props.pos.y = 25;
	props.size.w = 150;
	props.size.h = 25;
	widget_create(client, WIDGET_TYPE_INPUT, login, props, inp);

	input* inp1 = calloc(1, sizeof(input));
	inp1->confirm = click;
	inp1->placeholder = "Password";
	props.pos.x = 25;
	props.pos.y = 55;
	props.size.w = 150;
	props.size.h = 25;
	widget_create(client, WIDGET_TYPE_INPUT, login, props, inp1);

	button* butt = malloc(sizeof(button));
	butt->flags = 0;
	butt->label = "Login";
	butt->click = click;
	props.pos.x = 25;
	props.pos.y = 85;
	props.size.w = 150;
	props.size.h = 25;
	widget_create(client, WIDGET_TYPE_BUTTON, login, props, butt);

	widget_draw(login);

	while(1) {
		widgets_tick(login);
	}

	return 0;
}
