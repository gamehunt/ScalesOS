#include "events.h"
#include "compose.h"
#include "request.h"
#include <stdlib.h>

#include <string.h>
#include <unistd.h>

static list_t* __listeners[COMPOSE_EVENT_MAX] = {0};

void compose_cl_event_send_to_all(compose_client_t* client, compose_event_t* event) {
	compose_event_req_t* ereq = malloc(sizeof(compose_event_req_t));
	ereq->target = 0;
	memcpy(&ereq->event, event, sizeof(compose_event_t));

	compose_cl_send_request(client, ereq);
	free(ereq);
}

void compose_cl_event_send(compose_client_t* client, id_t id, compose_event_t* event) {
	compose_event_req_t* ereq = malloc(sizeof(compose_event_req_t));
	ereq->target = id;
	memcpy(&ereq->event, event, sizeof(compose_event_t));

	compose_cl_send_request(client, ereq);
	free(ereq);
}

void compose_sv_event_send(compose_client_t* cli, compose_event_t* event) {
	write(cli->socket, event, event->size);
}

void compose_sv_event_send_to_all(compose_server_t* srv, compose_event_t* event) {
	for(size_t i = 0; i < srv->clients->size; i++) {
		compose_sv_event_send(srv->clients->data[i], event);
	}
}

void compose_cl_add_event_handler(int type, event_listener listener) {
	if(type >= COMPOSE_EVENT_MAX) {
		return;
	}

	if(type < 0) {
		if(type == COMPOSE_EVENT_ALL) {
			for(int i = 0; i < COMPOSE_EVENT_MAX; i++) {
				compose_cl_add_event_handler(i, listener);
			}
		} else {
			return;
		}
	}

	list_t* queue = __listeners[type];
	if(!queue) {
		queue = list_create();
		__listeners[type] = queue;
	}
	list_push_back(queue, listener);
}

void compose_cl_handle_event(compose_client_t* client, compose_event_t* ev) {
	if(ev->type >= COMPOSE_EVENT_MAX) {
		return;
	}

	list_t* listeners = __listeners[ev->type];
	if(!listeners) {
		listeners = list_create();
		__listeners[ev->type] = listeners;
	}

	if(!listeners->size) {
		return;
	}

	for(int i = listeners->size - 1; i >= 0; i--) {
		int r = ((event_listener) listeners->data[i])(ev);
		if(r == COMPOSE_ACTION_BLOCK) {
			break;
		}
	}
}

compose_event_t* compose_cl_event_poll(compose_client_t* cli) {
	compose_request_t tmpev;

	if(read(cli->socket, &tmpev, sizeof(compose_request_t)) > 0) {
		compose_event_t* ev = malloc(tmpev.size);
		memcpy(ev, &tmpev, sizeof(compose_event_t));
		read(cli->socket, ev + 1, tmpev.size - sizeof(compose_event_t));
		return ev;
	}

	return NULL;
}
