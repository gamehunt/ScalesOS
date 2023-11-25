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

#define WIDGET_SIZE_POLICY_FIXED  0
#define WIDGET_SIZE_POLICY_EXPAND 1

#define WIDGET_SIZE_POLICY_V_SHIFT 16

#define WIDGET_SIZE_POLICY_H_MASK  (0xFFFF)
#define WIDGET_SIZE_POLICY_V_MASK  (0xFFFF0000)

#define WIDGET_SIZE_POLICY(h, v) ((h & 0xFFFF) | ((v & 0xFFFF) << WIDGET_SIZE_POLICY_V_SHIFT))
#define WIDGET_SIZE_POLICY_H(p)  (p & WIDGET_SIZE_POLICY_H_MASK)
#define WIDGET_SIZE_POLICY_V(p)  ((p & WIDGET_SIZE_POLICY_V_MASK) >> WIDGET_SIZE_POLICY_V_SHIFT)

#define WIDGET_LAYOUT_DIR_NONE 0
#define WIDGET_LAYOUT_DIR_V    1
#define WIDGET_LAYOUT_DIR_H    2

#define WIDGET_ALIGN_NONE    0
#define WIDGET_ALIGN_LEFT    (1 << 0)
#define WIDGET_ALIGN_RIGHT   (1 << 1)
#define WIDGET_ALIGN_HCENTER (1 << 2)
#define WIDGET_ALIGN_VCENTER (1 << 3)
#define WIDGET_ALIGN_TOP     (1 << 4)
#define WIDGET_ALIGN_BOTTOM  (1 << 5)

struct _widget;

typedef struct {
	void (*draw)(struct _widget* w);
	void (*release)(struct _widget* w);
	void (*process_event)(struct _widget* w, compose_event_t* ev);
} widget_ops;

typedef int32_t size_policy;
typedef int32_t alignment;
typedef int32_t layout_direction;

typedef struct {
	int top;
	int bottom;
	int left;
	int right;
} margins;

typedef margins padding;

typedef struct {
	position_t       pos;
	sizes_t          size;
	size_policy      size_policy;
	alignment        alignment;
	layout_direction layout_direction;
	margins          margins;
	padding          padding;
} widget_properties;

typedef struct _widget{
	uint16_t   type;
	id_t       win;
	widget_properties props;
	compose_client_t* client;
	widget_ops ops;
	compose_cl_gc_t*  ctx;
	void*      data;
	struct _widget* parent;
	list_t*    children;
} widget;

typedef void(*widget_init)(widget*);

void    widget_register(uint16_t type, widget_init init);

widget* widget_create(compose_client_t* cli, uint16_t type, widget* parent, widget_properties prop, void* data);
void    widget_release(widget* w);
void    widget_draw(widget* w);
void    widget_process_event(widget* w, compose_event_t* ev);

void    widgets_init();
void    widgets_tick(widget* root);
#endif
