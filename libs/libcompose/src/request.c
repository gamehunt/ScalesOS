#include "request.h"
#include "compose.h"
#include "events.h"
#include "render.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int compose_cl_send_request(compose_client_t* client, compose_request_t* req) {
	return write(client->socket, req, req->size);
}

void compose_sv_handle_request(compose_server_t* srv, compose_client_t* cli, compose_request_t* req) {
	compose_window_t*  win;
	compose_client_t*  trg;
	compose_win_req_t* wreq;
	switch(req->type) {
		case COMPOSE_REQ_MOVE:
			win = compose_sv_get_window(srv, ((compose_move_req_t*)req)->win);
			if(!win) {
				break;
			}
			compose_sv_move(win, ((compose_move_req_t*)req)->x, ((compose_move_req_t*)req)->y, ((compose_move_req_t*)req)->z);
			break;
		case COMPOSE_REQ_RESIZE:
			win = compose_sv_get_window(srv, ((compose_resize_req_t*)req)->win);
			if(!win) {
				break;
			}
			compose_sv_resize(win, ((compose_resize_req_t*)req)->w, ((compose_resize_req_t*)req)->h);
			break;
		case COMPOSE_REQ_CLOSE:
			compose_sv_close(srv, cli);
			break;
		case COMPOSE_REQ_DRAW:
			win = compose_sv_get_window(srv, ((compose_draw_req_t*)req)->win);
			if(!win) {
				break;
			}
			compose_sv_draw(win, ((compose_draw_req_t*)req)->op, ((compose_draw_req_t*)req)->params);
			break;
		case COMPOSE_REQ_EVENT:
			if(!((compose_event_req_t*)req)->target) {
				compose_sv_event_send_to_all(srv, &((compose_event_req_t*)req)->event);
			} else {
				trg = compose_sv_get_client(srv,  ((compose_event_req_t*)req)->target);
				if(!trg) {
					break;
				}
				compose_sv_event_send(trg, &((compose_event_req_t*)req)->event);
			}
			break;
		case COMPOSE_REQ_NEWWIN:
			wreq = (compose_win_req_t*) req;
			win  = compose_sv_get_window(srv, wreq->par);
			compose_sv_create_window(srv, cli, win, wreq->id, wreq->x, wreq->y, wreq->w, wreq->h, wreq->flags);
			break;
	}
}

compose_request_t* compose_sv_request_poll(compose_client_t* cli) {
	compose_request_t tmpreq;

	if(read(cli->socket, &tmpreq, sizeof(compose_request_t)) > 0) {
		compose_request_t* req = malloc(tmpreq.size);
		memcpy(req, &tmpreq, sizeof(compose_request_t));
		read(cli->socket, req + 1, tmpreq.size - sizeof(compose_request_t));
		return req;
	}

	return NULL;
}
