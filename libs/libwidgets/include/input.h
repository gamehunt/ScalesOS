#ifndef __LIB_WIDGETS_INPUT_H
#define __LIB_WIDGETS_INPUT_H

#include "compose/events.h"
#include "widget.h"

#define INPUT_TYPE_DEFAULT  0
#define INPUT_TYPE_PASSWORD 1

#define INPUT_FLAG_FOCUSED (1 << 0)

#define MAX_INPUT_SIZE 256

typedef struct {
	char   buffer[MAX_INPUT_SIZE];
	size_t buffer_size;

	const char* placeholder;
	int type;
	int flags;
	int cursor_pos;

	void (*confirm)(widget* w);
} input;

void input_init(widget* input);
void input_release(widget* input);
void input_draw(widget* input);
void input_process_events(widget* input, compose_event_t* ev);

#endif
