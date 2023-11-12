#ifndef __LIB_COMPOSE_REQUEST_H
#define __LIB_COMPOSE_REQUEST_H

#ifdef __COMPOSE
#include "compose.h"
#include "events.h"
#else
#include "compose/compose.h"
#include "compose/events.h"
#endif

#define COMPOSE_REQ_MOVE       1
#define COMPOSE_REQ_CLOSE_WIN  2
#define COMPOSE_REQ_RESIZE     3
#define COMPOSE_REQ_EVENT      4
#define COMPOSE_REQ_NEWWIN     5 
#define COMPOSE_REQ_DRAW       6
#define COMPOSE_REQ_EVMASK     7
#define COMPOSE_REQ_GRAB       8
#define COMPOSE_REQ_FOCUS      9
#define COMPOSE_REQ_DISCONNECT 10

typedef struct {
	int    type;
	size_t size;
} compose_request_t;

typedef struct {
	compose_request_t req;
	id_t win;
	int x;
	int y;
	int z;
} compose_move_req_t;

typedef struct {
	compose_request_t req;
	id_t win;
	int w;
	int h;
} compose_resize_req_t;

typedef struct {
	compose_request_t req;
	id_t target;
	compose_event_t event;
} compose_event_req_t;

typedef struct {
	compose_request_t   req;
	id_t                parent;
	window_properties_t props;	
} compose_win_req_t;

typedef struct {
	compose_request_t req;
	id_t              win;
	uint32_t          op;
	uint32_t          params[];
} compose_draw_req_t;

typedef struct {
	compose_request_t req;
	id_t 			  win;
	uint64_t          mask;
} compose_evmask_req_t;

typedef struct {
	compose_request_t req;
	id_t       		  win;
	grab_type 		  type;
} compose_grab_req_t;

typedef struct {
	compose_request_t req;
	id_t       		  win;
} compose_focus_req_t;

typedef struct {
	compose_request_t req;
} compose_disconnect_req_t;

int                compose_cl_send_request(compose_client_t* client, compose_request_t* req);

compose_request_t* compose_sv_request_poll(compose_client_t* cli);
void               compose_sv_handle_request(compose_server_t* srv, compose_client_t* cli, compose_request_t* req);

#endif
