#include "win.h"
#include "compose/compose.h"
#include "compose/render.h"
#include <stdlib.h>

void window_init(widget* window) {
	id_t par = window->client->root;
	if(window->parent) {
		par = window->parent->win;
	}
	window_properties_t props;
	props.x = 0;
	props.y = 0;
	props.w = 200;
	props.h = 100;
	props.border_width = 0;
	props.flags = COMPOSE_WIN_FLAGS_MOVABLE | COMPOSE_WIN_FLAGS_RESIZABLE;
	window->win = compose_cl_create_window(window->client, par, props);
	window->ops.release = window_release;
	window->ops.draw    = window_draw;
}

void window_release(widget* window) {
	free(window);
}

void window_draw(widget* window) {
	compose_cl_rect(window->client, window->win, 0, 0, 200 - 1, 100 - 1, 0xFFFF0000);
	compose_cl_flush(window->client, window->win);
}
