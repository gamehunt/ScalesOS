#ifndef __LIB_WIDGETS_LABEL_H
#define __LIB_WIDGETS_LABEL_H

#include "widget.h"

typedef struct {
	const char* text;
} label;

void label_init(widget* label);
void label_release(widget* label);
void label_draw(widget* label);
void label_process_events(widget* label, compose_event_t* ev);

#endif
