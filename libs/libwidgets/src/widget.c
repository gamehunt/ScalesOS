#include "widget.h"
#include "button.h"
#include "compose/compose.h"
#include "compose/events.h"
#include "label.h"
#include "win.h"
#include "input.h"
#include <stdlib.h>

static widget_init* __initializers = NULL;

void widget_register(uint16_t type, widget_init init) {
	if(!__initializers) {
		__initializers = calloc(1, 65535 * sizeof(widget_init));
	}

	__initializers[type] = init;
	// printf("Set 0x%.8x initializer for type %d\n", init, type);
}


void widget_release(widget* w) {
	if(w->parent) {
		list_delete_element(w->parent->children, w);
	}
	if(w->ops.release) {
		w->ops.release(w);
	}
	free(w);
}

void widget_draw(widget* w) {
	if(w->ops.draw) {
		w->ops.draw(w);
		for(size_t i = 0; i < w->children->size; i++) {
			widget* wd = w->children->data[i];
			widget_draw(wd);
		}
		compose_cl_flush(w->client, w->win);
	}
}

void widgets_init() {
	widget_register(WIDGET_TYPE_BUTTON, button_init);
	widget_register(WIDGET_TYPE_WINDOW, window_init);
	widget_register(WIDGET_TYPE_INPUT,  input_init);
	widget_register(WIDGET_TYPE_LABEL,  label_init);
}

int __widget_try_event(widget* w, compose_event_t* ev) {
	if(w->win == ev->win) {
		if(w->ops.process_event) {
			w->ops.process_event(w, ev);
		}
		return 1;
	}
	for(size_t i = 0; i < w->children->size; i++) {
		widget* child = w->children->data[i];
		if(__widget_try_event(child, ev)) {
			return 1;
		}	
	}
	return 0;
}

void widgets_tick(widget* root) {
	compose_event_t* ev = compose_cl_event_poll(root->client, 1);
	if(ev) {
		if(ev->type != COMPOSE_EVENT_KEEPALIVE && !__widget_try_event(root, ev)) {
			printf("%lld event for unknown widget: %d\n", ev->type, ev->win);
		}
		free(ev);
	}
}

void widget_handle_parent_resize(widget* parent, widget* child, int x, int y) {
	position_t old_pos = child->props.pos;
	sizes_t    old_sz  = child->props.size;

	int vsp = WIDGET_SIZE_POLICY_V(child->props.size_policy);
	int hsp = WIDGET_SIZE_POLICY_H(child->props.size_policy);
	switch(vsp) {
		case WIDGET_SIZE_POLICY_FIXED:
			if(parent->props.layout_direction != WIDGET_LAYOUT_DIR_V) {
				if(child->props.alignment & WIDGET_ALIGN_TOP) {
					child->props.pos.y = parent->props.padding.top;
				} else if(child->props.alignment & WIDGET_ALIGN_BOTTOM) {
					child->props.pos.y = parent->props.size.h - child->props.size.h - parent->props.padding.bottom;
				} else if(child->props.alignment & WIDGET_ALIGN_VCENTER) {
					child->props.pos.y = parent->props.padding.top + 
						(parent->props.size.h - parent->props.padding.top - parent->props.padding.bottom) / 2 
						- child->props.size.h / 2;
				}
			} else {
				child->props.pos.y = y;
			}
			break;
		case WIDGET_SIZE_POLICY_EXPAND:
			child->props.pos.y  = parent->props.padding.top;
			child->props.size.h = parent->props.size.h 
				- parent->props.padding.bottom - parent->props.padding.top;
			break;
	}
	switch(hsp) {
		case WIDGET_SIZE_POLICY_FIXED:
			if(parent->props.layout_direction != WIDGET_LAYOUT_DIR_H) {
				if(child->props.alignment & WIDGET_ALIGN_LEFT) {
					child->props.pos.x = parent->props.padding.left;
				} else if(child->props.alignment & WIDGET_ALIGN_RIGHT) {
					child->props.pos.x = parent->props.size.w - child->props.size.w - parent->props.padding.right;
				} else if(child->props.alignment & WIDGET_ALIGN_HCENTER) {
					child->props.pos.x = parent->props.padding.left + 
						(parent->props.size.w - parent->props.padding.left - parent->props.padding.right) / 2
						- child->props.size.w / 2;
				}
			} else {
				child->props.pos.x = x;
			}
			break;
		case WIDGET_SIZE_POLICY_EXPAND:
			child->props.pos.x  = parent->props.padding.left;
			child->props.size.w = parent->props.size.w - 
				parent->props.padding.right - parent->props.padding.left;
			break;
	}
	compose_cl_move(child->client, child->win, child->props.pos.x, child->props.pos.y);
	compose_cl_resize(child->client, child->win, child->props.size.w, child->props.size.h);
} 

void widget_handle_resize(widget* w, sizes_t new_sizes, int event) {
	if(event && w->ctx) {
		compose_cl_resize_gc(w->ctx, new_sizes);
	}
	w->props.size.w = new_sizes.w;
	w->props.size.h = new_sizes.h;
	int x = w->props.padding.left;
	int y = w->props.padding.top;
	for(size_t i = 0; i < w->children->size; i++) {
		widget* child = w->children->data[i];
		widget_handle_parent_resize(w, child, x, y);
		x += child->props.size.w + 5;
		y += child->props.size.h + 5;
	}
	if(w->ops.draw) {
		w->ops.draw(w);
	}
	if(event) {
		compose_cl_confirm_resize(w->client, w->win);
	}
}

void widget_process_event(widget* w, compose_event_t* ev) {
	if(ev->type == COMPOSE_EVENT_RESIZE) {
		compose_resize_event_t* e = (compose_event_t*) ev;
		widget_handle_resize(w, e->new_size, 1);
	}	
}

widget* widget_create(compose_client_t* cli, uint16_t type, widget* parent, widget_properties prop, void* data) {
	widget* w = calloc(1, sizeof(widget));
	w->type = type;
	w->data = data;
	w->parent = parent;
	w->props  = prop;
	w->client = cli;
	w->children = list_create();
	w->ops.process_event = widget_process_event;

	if(__initializers && __initializers[type]) {
		__initializers[type](w);
	}

	if(w->win && !w->ctx) {
		w->ctx = compose_cl_get_gc(w->win);
	}

	if(parent) {
		list_push_back(w->parent->children, w);
		widget_handle_resize(parent, parent->props.size, 0);
	}

	return w;
}

