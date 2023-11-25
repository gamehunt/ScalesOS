#include "compose/compose.h"
#include "compose/render.h"

#include "jpeg.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <widgets/widget.h>
#include <widgets/button.h>
#include <widgets/input.h>

#include <auth.h>
#include <cfg.h>

static compose_client_t* client = NULL;
static widget* window    = NULL;
static widget* login     = NULL;
static widget* password  = NULL;

extern char** environ;

void do_login() {
	char* login_text = ((input*) login->data)->buffer;
	char* passw_text = ((input*) password->data)->buffer;

	int r = auth_check_credentials(login_text, passw_text);

	if(r >= 0) {
		compose_cl_disconnect(client);
		
		cfg_data* data = cfg_open("/etc/lockscreen.cfg");
		char* startup  = cfg_get_value(data, "startup");
		char* exec = strdup(strtok(startup, " "));
		char** argv = NULL;
		int argc = 0;

		char* arg;
		while((arg = strtok(NULL, " "))) {
			if(!argv) {
				argv = malloc(sizeof(char*));
			} else {
				argv = realloc(argv, (argc + 1) * sizeof(char*));
			}
			argv[argc] = strdup(arg);
			argc++;
		}

		if(argv) {
			argv = realloc(argv, (argc + 1) * sizeof(char*));
			argv[argc] = 0;
		}

		auth_setenv(r);

		printf("Executing: %s\n", exec);

		execve(exec, argv, environ);
		perror("Exec failed");
		exit(1);
	} else {
		// TODO
	}
}

void click(widget* w) {
	do_login();
}

void confirm(widget* w) {
	if(w == login) {
		compose_cl_focus(client, password->win);
	} else {
		do_login();	
	}
}

int main(int argc, char** argv) {
	client = compose_cl_connect("/tmp/.compose.sock");
	if(!client) {
		perror("Failed to connect to compose server.");
		return 1;
	}

	jpeg_t* jpeg = jpeg_open("/res/background.jpg");

	if(jpeg) {
		compose_cl_gc_t* ctx = compose_cl_get_gc(client->root);
		coord_t x = 0;
		coord_t y = 0;
		fb_bitmap(&ctx->fb, x, y, jpeg->w, jpeg->h, 32, jpeg->data);
		compose_cl_flush(client, client->root);
		jpeg_close(jpeg);
	}

	widgets_init();

	widget_properties props = {0};
	props.pos.x = 500;
	props.pos.y = 300;
	props.size.w = 200;
	props.size.h = 125;
	props.padding.top   = 25;
	props.padding.left  = 5;
	props.padding.right = 5;
	props.layout_direction = WIDGET_LAYOUT_DIR_V;
	window = widget_create(client, WIDGET_TYPE_WINDOW, NULL, props, NULL);

	input* inp = calloc(1, sizeof(input));
	inp->confirm = confirm;
	inp->placeholder = "Login";
	props.size.h = 25;
	props.size_policy = WIDGET_SIZE_POLICY(WIDGET_SIZE_POLICY_EXPAND, WIDGET_SIZE_POLICY_FIXED);
	login = widget_create(client, WIDGET_TYPE_INPUT, window, props, inp);

	input* inp1 = calloc(1, sizeof(input));
	inp1->confirm = confirm;
	inp1->placeholder = "Password";
	inp1->type = INPUT_TYPE_PASSWORD;
	password = widget_create(client, WIDGET_TYPE_INPUT, window, props, inp1);

	button* butt = malloc(sizeof(button));
	butt->flags = 0;
	butt->label = "Login";
	butt->click = click;
	widget_create(client, WIDGET_TYPE_BUTTON, window, props, butt);

	widget_draw(window);

	while(1) {
		widgets_tick(window);
	}

	return 0;
}
