#include "events.h"
#include "compose.h"
#include "request.h"
#include <stdlib.h>

#include <string.h>
#include <unistd.h>

typedef struct {
	event_listener* listeners;
	size_t          listeners_amount;
} listener_queue;

static listener_queue __listeners[COMPOSE_EVENT_MAX] = {0};

compose_event_t* compose_event_create(event_type type, void* data) {
	compose_event_t* ev = malloc(sizeof(compose_event_t));
	ev->type = type;
	ev->data = data;
	return ev;
}

void compose_event_release(compose_event_t* ev) {
	free(ev);
}

void compose_client_event_send_to_all(compose_client_t* client, compose_event_t* event) {
	compose_event_req_t* ereq = malloc(sizeof(compose_event_req_t));
	ereq->target = 0;
	memcpy(&ereq->event, event, sizeof(compose_event_t));

	compose_request_t* req = compose_client_create_request(COMPOSE_REQ_EVENT, ereq);

	compose_client_send_request(client, req);
	free(ereq);
}

void compose_client_event_send(compose_client_t* client, id_t id, compose_event_t* event) {
	compose_event_req_t* ereq = malloc(sizeof(compose_event_req_t));
	ereq->target = id;
	memcpy(&ereq->event, event, sizeof(compose_event_t));

	compose_request_t* req = compose_client_create_request(COMPOSE_REQ_EVENT, ereq);

	compose_client_send_request(client, req);
	free(ereq);
}

void compose_server_event_send_to_all(compose_server_t* srv, compose_event_t* event) {
	for(size_t i = 0; i < srv->clients_amount; i++) {
		compose_client_t* cli = srv->clients[i];
		write(cli->socket, event, sizeof(compose_event_t));
	}
}

void compose_client_add_event_handler(int type, event_listener listener) {
	if(type >= COMPOSE_EVENT_MAX) {
		return;
	}

	if(type < 0) {
		if(type == COMPOSE_EVENT_ALL) {
			for(int i = 0; i < COMPOSE_EVENT_MAX; i++) {
				compose_client_add_event_handler(i, listener);
			}
		} else {
			return;
		}
	}

	listener_queue queue = __listeners[type];
	
	if(!queue.listeners) {
		queue.listeners = malloc(sizeof(event_listener));
	} else {
		queue.listeners = realloc(queue.listeners, sizeof(event_listener) * (queue.listeners_amount + 1));
	}
	queue.listeners[queue.listeners_amount] = listener;
	queue.listeners_amount++;
}

void compose_client_handle_event(compose_client_t* client, compose_event_t* ev) {
	if(ev->type >= COMPOSE_EVENT_MAX) {
		return;
	}

	listener_queue listeners = __listeners[ev->type];
	if(!listeners.listeners_amount) {
		return;
	}

	for(size_t i = listeners.listeners_amount - 1; i >= 0; i++) {
		int r = listeners.listeners[i](client, ev);
		if(r == COMPOSE_ACTION_BLOCK) {
			break;
		}
	}
}
