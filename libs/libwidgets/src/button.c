#include "button.h"
#include "input/mouse.h"
#include "widget.h"

#include "compose/compose.h"
#include "compose/events.h"
#include "compose/render.h"

#include <stdlib.h>

void button_init(widget* button_w) {
	id_t par = button_w->client->root;
	if(button_w->parent) {
		par = button_w->parent->win;
	}
	window_properties_t props;
	props.x = 25;
	props.y = 25;
	props.w = 150;
	props.h = 50;
	props.border_width = 0;
	props.flags = 0;
	props.event_mask = COMPOSE_EVENT_BUTTON;
	button_w->win = compose_cl_create_window(button_w->client, par, props);
	button_w->ops.release = button_release;
	button_w->ops.draw    = button_draw;
	button_w->ops.process_event = button_process_events;
}

void button_release(widget* button) {
	free(button->data);
}

void button_draw(widget* buttn) {
	button* b = buttn->data;
	if(b->flags & BUTTON_FLAG_CLICKED) {
		compose_cl_fill(buttn->client, buttn->win, 0xFF00FF00);
	} else {
		compose_cl_fill(buttn->client, buttn->win, 0xFF0000FF);
	}
}

void button_process_events(widget* buttn, compose_event_t* ev) {
	button* b = buttn->data;
	if(ev->type == COMPOSE_EVENT_BUTTON) {
		compose_mouse_event_t* mev = (compose_mouse_event_t*) ev;
		if(mev->packet.buttons & MOUSE_BUTTON_LEFT) {
			if(!(b->flags & BUTTON_FLAG_CLICKED)) {
				b->flags |= BUTTON_FLAG_CLICKED;
				if(b->click) {
					b->click(buttn);
				}
				widget_draw(buttn);
			}
		} else if(b->flags & BUTTON_FLAG_CLICKED) {
			b->flags &= ~BUTTON_FLAG_CLICKED;
			widget_draw(buttn);
		}
	}
}
