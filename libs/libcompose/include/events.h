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

#define COMPOSE_EVENT_ALL    -1

#define COMPOSE_EVENT_KEY    1
#define COMPOSE_EVENT_MOUSE  2
#define COMPOSE_EVENT_RESIZE 3
#define COMPOSE_EVENT_MOVE   4

#define COMPOSE_EVENT_MAX    255

#define COMPOSE_ACTION_CONT  0
#define COMPOSE_ACTION_BLOCK 1

typedef uint16_t event_type;

typedef struct {
	event_type type;
	size_t     size;
} compose_event_t;

typedef struct {
	compose_event_t   event;
	keyboard_packet_t packet;
} compose_key_event_t;

typedef struct {
	compose_event_t event;
	mouse_packet_t  packet;
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

typedef int (*event_listener)(compose_event_t* ev);

void             compose_cl_event_send(compose_client_t* client, id_t id, compose_event_t* event);
void             compose_cl_event_send_to_all(compose_client_t* client, compose_event_t* event);
compose_event_t* compose_cl_event_poll(compose_client_t* client);
void             compose_cl_handle_event(compose_client_t* client, compose_event_t* ev);
void             compose_cl_add_event_handler(int type, event_listener listener);

void             compose_sv_event_send(compose_client_t* cli, compose_event_t* event);
void             compose_sv_event_send_to_all(compose_server_t* srv, compose_event_t* event);

#endif
