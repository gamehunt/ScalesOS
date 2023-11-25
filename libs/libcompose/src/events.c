#include "events.h"
#include "compose.h"
#include "request.h"
#include <stdlib.h>

#include <string.h>
#include <unistd.h>

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

typedef struct {
	id_t client;
	list_t* queue;
} raised_event_queue;

static list_t* __raised_events = NULL;

void compose_cl_event_raise(compose_client_t* client, compose_event_t* ev) {
	if(!__raised_events) {
		__raised_events = list_create();
	}

	raised_event_queue* queue = NULL;
	for(size_t i = 0; i < __raised_events->size; i++) {
		raised_event_queue* q = __raised_events->data[i];
		if(q->client == client->id) {
			queue = q;
			break;
		}
	}

	if(!queue) {
		queue = malloc(sizeof(raised_event_queue));
		queue->client = client->id;
		queue->queue  = list_create();
		list_push_back(__raised_events, queue);
	}

	list_push_back(queue->queue, ev);
}

void compose_sv_event_send(compose_client_t* cli, compose_event_t* event) {
	write(cli->socket, event, event->size);
}

void compose_sv_event_send_to_all(compose_server_t* srv, compose_event_t* event) {
	for(size_t i = 0; i < srv->clients->size; i++) {
		compose_sv_event_send(srv->clients->data[i], event);
	}
}

compose_event_t* compose_cl_event_poll(compose_client_t* cli, int raised) {
	if(raised && __raised_events) {
		raised_event_queue* queue = NULL;

		for(size_t i = 0; i < __raised_events->size; i++) {
			raised_event_queue* q = __raised_events->data[i];
			if(q->client == cli->id) {
				queue = q;
				break;
			}
		}

		if(queue && queue->queue->size > 0) {
			compose_event_t* ev = list_pop_front(queue->queue);
			return ev;
		}
	}

	compose_event_t tmpev;

	if(read(cli->socket, &tmpev, sizeof(compose_event_t)) > 0) {
		compose_event_t* ev = malloc(tmpev.size);
		memcpy(ev, &tmpev, sizeof(compose_event_t));
		if(tmpev.size > sizeof(compose_event_t)) {
			read(cli->socket, ev + 1, tmpev.size - sizeof(compose_event_t));
		}
		return ev;
	}

	return NULL;
}

static grab_t* __get_grab(compose_server_t* srv, compose_window_t* root, compose_event_t* event) {
	for(size_t i = 0; i < srv->grabs->size; i++) {
		grab_t* gr = srv->grabs->data[i];
		if(gr->type & event->type && gr->window->id == root->id) {
			return gr;
		}
	}
	return NULL;
}

void compose_sv_event_propagate(compose_server_t* srv, compose_window_t* root, compose_event_t* event) {
	grab_t* gr = __get_grab(srv, root, event);
	if((root->event_mask & event->type) || gr) {
		event->win = root->id;
		if((root->event_mask & event->type) && root->client) {
			compose_sv_event_send(root->client, event);
		}
		if(gr) {
			compose_sv_event_send(gr->client, event);
		}
	} 
	if(root->parent) {
		event->child = root->id;
		compose_sv_event_propagate(srv, root->parent, event);
	}
}
