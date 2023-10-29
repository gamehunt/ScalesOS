#ifndef __LIB_COMPOSE_REQUEST_H
#define __LIB_COMPOSE_REQUEST_H

#include "events.h"
#ifdef __COMPOSE
#include "compose.h"
#else
#include "compose/compose.h"
#endif

#define COMPOSE_REQ_REDRAW 1
#define COMPOSE_REQ_MOVE   2
#define COMPOSE_REQ_CLOSE  3
#define COMPOSE_REQ_RESIZE 4
#define COMPOSE_REQ_EVENT  5

typedef struct {
	int   type;
	void* data;
} compose_request_t;

typedef struct {
	int x;
	int y;
	int z;
} compose_move_req_t;

typedef struct {
	int w;
	int h;
} compose_resize_req_t;

typedef struct {
	id_t target;
	compose_event_t event;
} compose_event_req_t;

compose_request_t* compose_client_create_request(int type, void* data);
void               compose_client_release_request(compose_request_t*);
int                compose_client_send_request(compose_client_t* client, compose_request_t* req);

compose_request_t* compose_server_request_poll(compose_client_t* cli);
void               compose_server_handle_request(compose_server_t* srv, compose_client_t* cli, compose_request_t* req);

#endif
