#include "label.h"

void label_init(widget* label_w) {
	id_t par = label_w->client->root;
	if(label_w->parent) {
		par = label_w->parent->win;
	}
	window_properties_t props;
	props.x = label_w->props.pos.x;
	props.y = label_w->props.pos.y;
	props.w = label_w->props.size.w;
	props.h = label_w->props.size.h;
	props.border_width = 0;
	props.flags        = 0;
	label_w->win       = compose_cl_create_window(label_w->client, par, props);
	label_w->ops.release 	   = label_release;
	label_w->ops.draw    	   = label_draw;
	label_w->ops.process_event = label_process_events;
}

void label_release(widget* label) {

}

void label_draw(widget* l) {
	label* lbl = l->data;
	fb_string(&l->ctx->fb, 5, 5, lbl->text, NULL, 0x00, 0xFF000000);
}

void label_process_events(widget* label, compose_event_t* ev) {
	widget_process_event(label, ev);
}
