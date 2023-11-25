#include "win.h"
#include "compose/compose.h"
#include "compose/events.h"
#include <stdlib.h>

void window_init(widget* window) {
	id_t par = window->client->root;
	if(window->parent) {
		par = window->parent->win;
	}
	window_properties_t props;
	props.x = window->props.pos.x;
	props.y = window->props.pos.y;
	props.w = window->props.size.w;
	props.h = window->props.size.h;
	props.border_width = 0;
	props.flags = COMPOSE_WIN_FLAGS_MOVABLE | COMPOSE_WIN_FLAGS_RESIZABLE;
	props.event_mask = COMPOSE_EVENT_RESIZE;
	window->win = compose_cl_create_window(window->client, par, props);
	window->ops.release = window_release;
	window->ops.draw    = window_draw;
	window->ops.process_event = window_process_events;

	compose_cl_focus(window->client, window->win);
}

void window_release(widget* window) {
	free(window);
}

void window_draw(widget* window) {
	fb_rect(&window->ctx->fb, 0, 0, window->props.size.w - 1, window->props.size.h - 1, 0xFFFF0000);
}

void window_process_events(widget* win, compose_event_t* ev) {
	widget_process_event(win, ev);
}
