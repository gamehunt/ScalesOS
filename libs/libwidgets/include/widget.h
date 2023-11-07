#ifndef __LIB_WIDGETS_WIDGET_H
#define __LIB_WIDGETS_WIDGET_H

#include "compose/compose.h"
#include "compose/events.h"
#include "types/list.h"
#include <stdint.h>

#define WIDGET_TYPE_BUTTON 1
#define WIDGET_TYPE_LABEL  2
#define WIDGET_TYPE_INPUT  3
#define WIDGET_TYPE_WINDOW 4

struct _widget;

typedef struct {
	void (*draw)(struct _widget* w);
	void (*release)(struct _widget* w);
	void (*process_event)(struct _widget* w, compose_event_t* ev);
} widget_ops;

typedef struct _widget{
	uint16_t   type;
	id_t       win;
	compose_client_t* client;
	widget_ops ops;
	void*      data;
	struct _widget* parent;
	list_t*    children;
} widget;

typedef void(*widget_init)(widget*);

void    widget_register(uint16_t type, widget_init init);

widget* widget_create(compose_client_t* cli, uint16_t type, widget* parent, void* data);
void    widget_release(widget* w);
void    widget_draw(widget* w);

void    widgets_init();
void    widgets_tick(widget* root);
#endif
