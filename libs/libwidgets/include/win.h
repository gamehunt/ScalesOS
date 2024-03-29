#ifndef __LIB_WIDGETS_WIN_H
#define __LIB_WIDGETS_WIN_H

#include "widget.h"

void window_init(widget* window);
void window_release(widget* window);
void window_draw(widget* window);
void window_process_events(widget* win, compose_event_t* ev);

#endif

