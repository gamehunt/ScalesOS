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
	props.x = button_w->props.pos.x;
	props.y = button_w->props.pos.y;
	props.w = button_w->props.size.w;
	props.h = button_w->props.size.h;
	props.border_width = 0;
	props.flags = 0;
	props.event_mask = COMPOSE_EVENT_BUTTON;
	button_w->win = compose_cl_create_window(button_w->client, par, props);
	button_w->ops.release = button_release;
	button_w->ops.draw    = button_draw;
	button_w->ops.process_event = button_process_events;
}

void button_release(widget* button) {
}

void button_draw(widget* buttn) {
	button* b = buttn->data;
	if(b->flags & BUTTON_FLAG_CLICKED) {
		compose_cl_fill(buttn->client, buttn->win, 0xFF00FF00);
	} else {
		compose_cl_fill(buttn->client, buttn->win, 0xFF0000FF);
	}

	compose_cl_string(buttn->client, buttn->win, 5, 5, 0x00, 0xFF000000, b->label);
}

void button_process_events(widget* buttn, compose_event_t* ev) {
	widget_process_event(buttn, ev);
	button* b = buttn->data;
	if(ev->type == COMPOSE_EVENT_BUTTON) {
		compose_mouse_event_t* mev = (compose_mouse_event_t*) ev;
		if(mev->packet.buttons & MOUSE_BUTTON_LEFT) {
			compose_cl_focus(buttn->client, buttn->win);
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
