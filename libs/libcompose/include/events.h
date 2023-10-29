#ifndef __LIB_COMPOSE_EVENTS_H
#define __LIB_COMPOSE_EVENTS_H

#ifdef __COMPOSE
#include "compose.h"
#else
#include "compose/compose.h"
#endif

#include <stdint.h>

#define COMPOSE_EVENT_ALL    -1

#define COMPOSE_EVENT_KEY    1
#define COMPOSE_EVENT_MOUSE  2
#define COMPOSE_EVENT_PAINT  3
#define COMPOSE_EVENT_RESIZE 4
#define COMPOSE_EVENT_MOVE   5

#define COMPOSE_EVENT_MAX    255

#define COMPOSE_ACTION_CONT  0
#define COMPOSE_ACTION_BLOCK 1

typedef uint16_t event_type;

typedef struct {
	event_type type;
	void*      data;
} compose_event_t;

typedef int (*event_listener)(compose_client_t* cli, compose_event_t* ev);

compose_event_t* compose_event_create(event_type type, void* data);
void             compose_event_release(compose_event_t* ev);

void             compose_client_event_send(compose_client_t* client, id_t id, compose_event_t* event);
void             compose_client_event_send_to_all(compose_client_t* client, compose_event_t* event);
compose_event_t* compose_client_event_poll(compose_client_t* client);
void             compose_client_handle_event(compose_client_t* client, compose_event_t* ev);
void             compose_client_add_event_handler(int type, event_listener listener);

void             compose_server_event_send_to_all(compose_server_t* srv, compose_event_t* event);

#endif
