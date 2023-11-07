#ifndef __LIB_COMPOSE_EVENTS_H
#define __LIB_COMPOSE_EVENTS_H

#include "input/kbd.h"
#include "input/mouse.h"

#ifdef __COMPOSE
#include "compose.h"
#else
#include "compose/compose.h"
#endif

#include <stdint.h>

#define COMPOSE_EVENT_KEY    (1 << 0)
#define COMPOSE_EVENT_MOUSE  (1 << 1)
#define COMPOSE_EVENT_BUTTON (1 << 2)
#define COMPOSE_EVENT_RESIZE (1 << 3)
#define COMPOSE_EVENT_MOVE   (1 << 4)
#define COMPOSE_EVENT_WIN    (1 << 5)
#define COMPOSE_EVENT_CNN    (1 << 6)

typedef uint16_t event_type;

typedef struct {
	event_type type;
	size_t     size;
	id_t       win;
	id_t       child;
	id_t       root;
} compose_event_t;

typedef struct {
	compose_event_t   event;
	keyboard_packet_t packet;
} compose_key_event_t;

typedef struct {
	compose_event_t event;
	mouse_packet_t  packet;
	int x, y;
	int abs_x, abs_y;
} compose_mouse_event_t;

typedef struct {
	compose_event_t event;
	position_t old_pos;
	position_t new_pos;
} compose_move_event_t;

typedef struct {
	compose_event_t event;
	sizes_t old_size;
	sizes_t new_size;
} compose_resize_event_t;

typedef struct {
	compose_event_t event;
} compose_win_event_t;

void             compose_cl_event_send(compose_client_t* client, id_t id, compose_event_t* event);
void             compose_cl_event_send_to_all(compose_client_t* client, compose_event_t* event);
compose_event_t* compose_cl_event_poll(compose_client_t* client);
void             compose_cl_event_mask(event_mask_t mask);

void             compose_sv_event_propagate(compose_window_t* root, compose_event_t* event);
void             compose_sv_event_propagate_to_grabs(list_t* grabs, compose_event_t* event);
void             compose_sv_event_send(compose_client_t* cli, compose_event_t* event);
void             compose_sv_event_send_to_all(compose_server_t* srv, compose_event_t* event);

#endif
