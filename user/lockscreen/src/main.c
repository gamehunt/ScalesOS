#include "compose/compose.h"
#include "compose/events.h"
#include "compose/render.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static compose_client_t* client = NULL;
static id_t winA = 0;
static id_t winB = 0;

void paint() {
	compose_cl_fill(client, winA, 0xFFFF0000);
	compose_cl_flush(client, winA);
	compose_cl_fill(client, winB, 0xFF0000FF);
	compose_cl_flush(client, winB);
}

int handle_event(compose_event_t* ev) {
	paint();
	return COMPOSE_ACTION_CONT;
}

int main(int argc, char** argv) {
	client = compose_cl_connect("/tmp/.compose.sock");
	if(!client) {
		perror("Failed to connect to compose server.");
		return 1;
	}

	window_properties_t props;
	props.x = 100;
	props.y = 100;
	props.w = 200;
	props.h = 100;
	props.border_width = 10;
	props.flags = COMPOSE_WIN_FLAGS_MOVABLE | COMPOSE_WIN_FLAGS_RESIZABLE;

	winA = compose_cl_create_window(client, 0, props);

	while(1) {
		compose_event_t* ev = compose_cl_event_poll(client);
		if(ev) {
			int t = ev->type;
			free(ev);
			if(t == COMPOSE_EVENT_WIN) {
				break;
			} 
		}
	}
	
	props.x = 150;
	props.y = 150;
	
	winB = compose_cl_create_window(client, 0, props);

	while(1) {
		compose_event_t* ev = compose_cl_event_poll(client);
		if(ev) {
			int t = ev->type;
			free(ev);
			if(t == COMPOSE_EVENT_WIN) {
				break;
			} 
		}
	}

	paint();
	compose_cl_add_event_handler(COMPOSE_EVENT_ALL, handle_event);

	while(1) {
		compose_event_t* ev = compose_cl_event_poll(client);
		if(ev) {
			compose_cl_handle_event(client, ev);
			free(ev);
		}
	}

	return 0;
}