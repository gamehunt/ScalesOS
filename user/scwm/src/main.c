#include "compose/compose.h"
#include "compose/events.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {	

	compose_client_t* client = compose_cl_connect("/tmp/.compose.sock");
	if(!client) {
		return -1;
	}

	compose_cl_grab(client, 0, COMPOSE_EVENT_KEY | COMPOSE_EVENT_MOUSE);

	while(1) {
		compose_event_t* ev = compose_cl_event_poll(client);
		if(ev) {
			free(ev);
		}
	}

	return 0;
}
