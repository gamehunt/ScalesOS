#include "compose/compose.h"
#include "compose/events.h"
#include "compose/render.h"

#include "input/keys.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv) {	
	if(!fork()) {
		execve("/bin/terminal", NULL, NULL);
		exit(1);
	}

	compose_client_t* client = compose_cl_connect("/tmp/.compose.sock");
	if(!client) {
		return -1;
	}

	compose_cl_fill(client, client->root, 0xFF000000);
	compose_cl_grab(client, client->root, COMPOSE_EVENT_KEY | COMPOSE_EVENT_BUTTON | COMPOSE_EVENT_MOUSE);

	int mod = 0;

	int mov          = 0;

	window_properties_t root_props = compose_cl_get_properties(client, client->root);
	window_properties_t win_props  = {0};

	while(1) {
		compose_event_t* ev = compose_cl_event_poll(client);
		if(ev) {
			switch(ev->type) {
				case COMPOSE_EVENT_KEY:
					if(((compose_key_event_t*) ev)->packet.scancode == KEY_LEFTMETA) {
						mod = !(((compose_key_event_t*) ev)->packet.flags & KBD_EVENT_FLAG_UP);
						compose_cl_focus(client, client->root);
					}
					break;
				case COMPOSE_EVENT_BUTTON:
					if(mod) {
						if(((compose_mouse_event_t*) ev)->packet.buttons & MOUSE_BUTTON_LEFT) {
							mov     = ev->child;
							win_props = compose_cl_get_properties(client, ev->child);
						} else {
							mov = 0;
						}
					}
					break;
				case COMPOSE_EVENT_MOUSE:
					if(mov) {
						win_props.x += ((compose_mouse_event_t*) ev)->packet.dx;
						win_props.y -= ((compose_mouse_event_t*) ev)->packet.dy;
						if(win_props.x >= root_props.w) {
							win_props.x = root_props.w;
						} else if(win_props.x < 0) {
							win_props.x = 0;
						}
						if(win_props.y >= root_props.h) {
							win_props.y = root_props.h;
						} else if(win_props.y < 0) {
							win_props.y = 0;
						}
						compose_cl_move(client, mov, win_props.x, win_props.y);
					}	
					break;
			}
			free(ev);
		}
	}

	return 0;
}
