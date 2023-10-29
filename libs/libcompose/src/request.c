#include "request.h"
#include "compose.h"
#include "events.h"

#include <stdlib.h>
#include <unistd.h>

compose_request_t* compose_client_create_request(int type, void* data) {
	compose_request_t* req = malloc(sizeof(compose_request_t));
	req->type = type;
	req->data = data;
	return req;
}

void compose_client_release_request(compose_request_t* req) {
	free(req);
}

int compose_client_send_request(compose_client_t* client, compose_request_t* req) {
	return write(client->socket, req, sizeof(compose_request_t));
}

void compose_server_handle_request(compose_server_t* srv, compose_client_t* cli, compose_request_t* req) {
	switch(req->type) {
		case COMPOSE_REQ_MOVE:
			cli->pos.x = ((compose_move_req_t*)req->data)->x;
			cli->pos.y = ((compose_move_req_t*)req->data)->y;
			break;
		case COMPOSE_REQ_RESIZE:
			cli->sizes.w = ((compose_resize_req_t*)req->data)->w;
			cli->sizes.h = ((compose_resize_req_t*)req->data)->h;
			break;
		case COMPOSE_REQ_CLOSE:
			compose_server_close(srv, cli);
			break;
		case COMPOSE_REQ_REDRAW:
			compose_server_redraw(cli);
			break;
		case COMPOSE_REQ_EVENT:
			if(!((compose_event_req_t*)req->data)->target) {
				compose_server_event_send_to_all(srv, &((compose_event_req_t*)req->data)->event);
			} else {

			}
			break;
	}
}

compose_request_t* compose_server_request_poll(compose_client_t* cli) {
	compose_request_t tmpreq;

	if(read(cli->socket, &tmpreq, sizeof(compose_request_t)) > 0) {
		compose_request_t* req = malloc(sizeof(compose_request_t));
		return req;
	}

	return NULL;
}
