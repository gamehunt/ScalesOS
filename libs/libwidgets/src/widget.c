#include "widget.h"
#include "button.h"
#include "compose/compose.h"
#include "compose/events.h"
#include "win.h"
#include "input.h"
#include <stdlib.h>

static widget_init* __initializers = NULL;

void widget_register(uint16_t type, widget_init init) {
	if(!__initializers) {
		__initializers = calloc(1, 65535 * sizeof(widget_init));
	}

	__initializers[type] = init;

	printf("Set 0x%.8x initializer for type %d\n", init, type);
}

widget* widget_create(compose_client_t* cli, uint16_t type, widget* parent, widget_properties prop, void* data) {
	widget* w = calloc(1, sizeof(widget));
	w->type = type;
	w->data = data;
	w->parent = parent;
	w->props  = prop;
	w->client = cli;
	w->children = list_create();
	if(parent) {
		list_push_back(w->parent->children, w);
	}

	if(__initializers && __initializers[type]) {
		__initializers[type](w);
	}

	return w;
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
	}
}

void widgets_init() {
	widget_register(WIDGET_TYPE_BUTTON, button_init);
	widget_register(WIDGET_TYPE_WINDOW, window_init);
	widget_register(WIDGET_TYPE_INPUT,  input_init);
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
	compose_event_t* ev = compose_cl_event_poll(root->client);
	if(ev) {
		__widget_try_event(root, ev);
		free(ev);
	}
}
