#ifndef __LIB_COMPOSE_REQUEST_H
#define __LIB_COMPOSE_REQUEST_H

#include "events.h"
#ifdef __COMPOSE
#include "compose.h"
#else
#include "compose/compose.h"
#endif

#define COMPOSE_REQ_DRAW   1
#define COMPOSE_REQ_MOVE   2
#define COMPOSE_REQ_CLOSE  3
#define COMPOSE_REQ_RESIZE 4
#define COMPOSE_REQ_EVENT  5

typedef struct {
	int    type;
	size_t size;
} compose_request_t;

typedef struct {
	compose_request_t req;
	int x;
	int y;
	int z;
} compose_move_req_t;

typedef struct {
	compose_request_t req;
	int w;
	int h;
} compose_resize_req_t;

typedef struct {
	compose_request_t req;
	id_t target;
	compose_event_t event;
} compose_event_req_t;

typedef struct {
	compose_request_t req;
	uint32_t          op;
	uint32_t          params[];
} compose_draw_req_t;

int                compose_cl_send_request(compose_client_t* client, compose_request_t* req);

compose_request_t* compose_sv_request_poll(compose_client_t* cli);
void               compose_sv_handle_request(compose_server_t* srv, compose_client_t* cli, compose_request_t* req);

#endif
