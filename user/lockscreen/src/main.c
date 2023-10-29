#include "compose/compose.h"
#include "compose/event.h"
#include "sys/un.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void handle_event(compose_event_t* ev) {

}

int main(int argc, char** argv) {

	compose_client_t* client = compose_connect("/tmp/.compose.sock");

	while(1) {
		compose_event_t* ev = compose_client_event_poll(client);
		if(ev) {
			handle_event(ev);
			compose_event_release(ev);
		}
	}

	return 0;
}
