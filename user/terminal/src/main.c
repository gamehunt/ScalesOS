#include "compose/compose.h"

#include "widgets/input.h"
#include "widgets/widget.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static compose_client_t* client = NULL;
static widget* window    = NULL;

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

	input* inp = calloc(1, sizeof(input));
	inp->placeholder = "Test";
	props.size.h      = 25;
	props.alignment   = WIDGET_ALIGN_VCENTER;
	props.size_policy = WIDGET_SIZE_POLICY(WIDGET_SIZE_POLICY_EXPAND, WIDGET_SIZE_POLICY_FIXED);
	widget* login = widget_create(client, WIDGET_TYPE_INPUT, window, props, inp);

	widget_draw(window);

	while(1) {
		widgets_tick(window);
	}

	return 0;

}
