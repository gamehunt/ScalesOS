#ifndef __LIB_WIDGETS_BUTTON_H
#define __LIB_WIDGETS_BUTTON_H

#include "compose/events.h"
#include "widget.h"

#define BUTTON_FLAG_CLICKED (1 << 0)

typedef struct {
	int flags;
	const char* label;
	void (*click)(widget* button);
} button;

void button_init(widget* button);
void button_release(widget* button);
void button_draw(widget* button);
void button_process_events(widget* button, compose_event_t* ev);

#endif
