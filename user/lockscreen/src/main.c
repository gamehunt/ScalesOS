#include "compose/compose.h"
#include "compose/events.h"
#include "compose/render.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static compose_client_t* client = NULL;

void paint() {
	compose_cl_fill(client, 0xFFFF0000);
	compose_cl_flush(client);
}

int handle_event(compose_event_t* ev) {
	paint();
	return COMPOSE_ACTION_CONT;
}

int main(int argc, char** argv) {

	client = compose_connect("/tmp/.compose.sock");

	compose_cl_resize(client, 200, 100);
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
