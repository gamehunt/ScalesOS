#ifndef __LIB_WIDGETS_INPUT_H
#define __LIB_WIDGETS_INPUT_H

#include "compose/events.h"
#include "widget.h"

typedef struct {
	char   buffer[128];
	size_t buffer_offset;

	const char* placeholder;

	void (*confirm)(widget* w);
} input;

void input_init(widget* input);
void input_release(widget* input);
void input_draw(widget* input);
void input_process_events(widget* input, compose_event_t* ev);

#endif
