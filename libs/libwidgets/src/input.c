#include "input.h"
#include "compose/events.h"
#include "compose/render.h"
#include "input/mouse.h"
#include "widget.h"

#include <input/keys.h>

void input_init(widget* input_w) {
	id_t par = input_w->client->root;
	if(input_w->parent) {
		par = input_w->parent->win;
	}
	window_properties_t props;
	props.x = input_w->props.pos.x;
	props.y = input_w->props.pos.y;
	props.w = input_w->props.size.w;
	props.h = input_w->props.size.h;
	props.border_width = 0;
	props.flags = 0;
	props.event_mask = COMPOSE_EVENT_KEY | COMPOSE_EVENT_BUTTON | COMPOSE_EVENT_FOCUS | COMPOSE_EVENT_UNFOCUS;
	input_w->win = compose_cl_create_window(input_w->client, par, props);
	input_w->ops.release 	   = input_release;
	input_w->ops.draw    	   = input_draw;
	input_w->ops.process_event = input_process_events;

}

void input_release(widget* input) {
}

void input_draw(widget* inp) {
	input* b = inp->data;
	compose_cl_fill(inp->client, inp->win, 0xFFFFFFFF);
	if(b->flags & INPUT_FLAG_FOCUSED) {
		compose_cl_line(inp->client, inp->win, 5 + 8 * b->buffer_offset, 5, 5 + 8 * b->buffer_offset, inp->props.size.h - 5, 0xFF000000);
	}
	if(b->buffer_offset) {
		if(b->type == INPUT_TYPE_DEFAULT) {
			compose_cl_string(inp->client, inp->win, 5, 5, 0x00, 0xFF000000, b->buffer);
		} else {
			char buff[128];
			for(int i = 0 ; i < b->buffer_offset; i++) {
				buff[i] = '*';
			}
			buff[b->buffer_offset] = '\0';
			compose_cl_string(inp->client, inp->win, 5, 5, 0x00, 0xFF000000, buff);
		}
	} else if(!(b->flags & INPUT_FLAG_FOCUSED)){
		compose_cl_string(inp->client, inp->win, 5, 5, 0x00, 0xFF999999, b->placeholder);
	}
}

void input_process_events(widget* inp, compose_event_t* ev) {
	input* b = inp->data;
	if(ev->type == COMPOSE_EVENT_KEY) {
		compose_key_event_t* mev = (compose_key_event_t*) ev;
		if(mev->packet.flags & KBD_EVENT_FLAG_UP) {
			return;
		}
		if(b->buffer_offset >= 128) {
			return;
		}
		if(mev->packet.scancode == KEY_ENTER) {
			if(b->confirm) {
				b->confirm(inp);
			}
		} else if(mev->packet.scancode == KEY_BACKSPACE) {
			if(b->buffer_offset) {
				b->buffer_offset--;
				b->buffer[b->buffer_offset] = '\0';
				widget_draw(inp);
			}
		} else {
			char t = mev->translated;
			if(t) {
				b->buffer[b->buffer_offset] = t;
				b->buffer_offset++;
				widget_draw(inp);
			}
		}
	} else if(ev->type == COMPOSE_EVENT_BUTTON) {
		compose_mouse_event_t* mev = (compose_mouse_event_t*) ev;
		if(mev->packet.buttons & MOUSE_BUTTON_LEFT) {
			compose_cl_focus(inp->client, inp->win);
		}
	} else if(ev->type == COMPOSE_EVENT_FOCUS) {
		b->flags |= INPUT_FLAG_FOCUSED;
		widget_draw(inp);
	} else if(ev->type == COMPOSE_EVENT_UNFOCUS) {
		b->flags &= ~INPUT_FLAG_FOCUSED;
		widget_draw(inp);
	}
}
