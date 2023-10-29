#include "compose.h"
#include "events.h"
#include "fb.h"
#include "kernel/dev/ps2.h"
#include "request.h"
#include "sys/select.h"
#include "sys/un.h"
#include "input/kbd.h"
#include "input/mouse.h"

#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

compose_server_t* compose_server_create(const char* sock) {
	int sockfd = socket(AF_LOCAL, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if(sockfd < 0) {
		return NULL;
	}

	struct sockaddr_un un;
	un.sun_family = AF_LOCAL;
	strncpy(un.sun_path, sock, sizeof(un.sun_path));

	if(bind(sockfd, &un, sizeof(un)) < 0) {
		return NULL;
	}

	compose_server_t* srv = calloc(1, sizeof(compose_server_t));
	srv->socket           = sockfd;
	
	int r =	fb_open("/dev/fb", &srv->framebuffer);
	if(r < 0) {
		close(srv->socket);
		free(srv);
		return NULL;
	}

	srv->devices[0] = open("/dev/kbd", O_RDONLY | O_NOBLOCK);
	srv->devices[1] = open("/dev/mouse", O_RDONLY | O_NOBLOCK);
	
	return srv;
}

compose_client_t* compose_server_create_client(int sock) {
	compose_client_t* cli = calloc(1, sizeof(compose_client_t));
	cli->socket = sock;
	return cli;
}

static compose_client_t* __compose_get_client(compose_server_t* srv, int sock) {
	for(size_t i = 0; i < srv->clients_amount; i++) {
		if(srv->clients[i]->socket == sock) {
			return srv->clients[i];
		}
	}
	return NULL;
}

static void __compose_add_client(compose_server_t* srv, int clifd) {
	if(__compose_get_client(srv, clifd) != NULL) {
		return;
	}
	compose_client_t* client = compose_server_create_client(clifd);
	if(!srv->clients) {
		srv->clients = malloc(sizeof(compose_client_t) * (srv->clients_amount + 1));	
	} else {
		srv->clients = realloc(srv->clients, sizeof(compose_client_t) * (srv->clients_amount + 1));	
	}
	srv->clients[srv->clients_amount] = client;
	srv->clients_amount++;
}

static void __compose_remove_client(compose_server_t* srv, compose_client_t* cli) {
    for(uint32_t i = 0; i < srv->clients_amount; i++){
        if(srv->clients[i] == cli){
            srv->clients[i] = 0;
            srv->clients_amount--;
            if(srv->clients_amount){
				if(i != srv->clients_amount) {
                	memmove(srv->clients + i, srv->clients + i + 1, (srv->clients_amount - i) * sizeof(void*));
				}
                srv->clients = realloc(srv->clients, srv->clients_amount * sizeof(void*));
            }else{
                free(srv->clients);
				srv->clients = 0;
            }
            break;
        }
    }
}

void compose_server_close(compose_server_t* srv, compose_client_t* cli) {
	__compose_remove_client(srv, cli);
	close(cli->socket);
	free(cli);
}

void compose_server_tick(compose_server_t* srv) {
	fd_set rset;
	FD_ZERO(&rset);

	int n = srv->socket;
	FD_SET(srv->socket, &rset);

	for(int i = 0; i < COMPOSE_DEVICE_AMOUNT; i++) {
		if(n > srv->devices[i]) {
			n = srv->devices[i];
		}
		FD_SET(srv->devices[i], &rset);
	}

	for(size_t i = 0; i < srv->clients_amount; i++) {
		compose_client_t* cli = srv->clients[i];
		if(cli->socket > n) {
			n = cli->socket;
		}
		FD_SET(cli->socket, &rset);
	}

	int r = select(n + 1, &rset, NULL, NULL, NULL);
	if(r <= 0) {
		return;
	}

	if(FD_ISSET(srv->socket, &rset)) {
		int clifd = 0;
		while((clifd = accept(srv->socket, NULL, NULL)) > 0) {
			__compose_add_client(srv, clifd);	
		}
	}

	for(size_t i = 0; i < srv->clients_amount; i++) {
		compose_client_t* cli = srv->clients[i];
		if(FD_ISSET(cli->socket, &rset)) {
			compose_request_t* req;
			while((req = compose_server_request_poll(cli))) {
				compose_server_handle_request(srv, cli, req);
				free(req);
			}
		}
	}

	for(int i = 0; i < COMPOSE_DEVICE_AMOUNT; i++) {
		if(FD_ISSET(srv->devices[i], &rset)) {
			while(1) {
				compose_event_t* event = NULL;
				void* raw_data;
				int   e = 0;
				switch(i) {
					case COMPOSE_DEVICE_KBD:
						e = read(srv->devices[i], &raw_data, 1);
						if(e <= 0) {
							break;
						}
						event = compose_event_create(COMPOSE_EVENT_KEY, input_kbd_create_packet((int) raw_data));
						break;
					case COMPOSE_DEVICE_MOUSE:
						raw_data = malloc(sizeof(ps_mouse_packet_t));
						e = read(srv->devices[i], raw_data, sizeof(ps_mouse_packet_t));
						if(e <= 0) {
							free(raw_data);
							break;
						}
						event = compose_event_create(COMPOSE_EVENT_MOUSE, input_mouse_create_packet(raw_data));
						free(raw_data);
						break;
				}
				if(event) {
					compose_server_event_send_to_all(srv, event);
					if(event->data) {
						free(event->data);
					}
					compose_event_release(event);
				} else {
					break;
				}
			}
		}
	}
}

void compose_server_redraw(compose_client_t* client) {

}

int compose_client_move(compose_client_t* cli, int x, int y) {
	compose_move_req_t* payload = malloc(sizeof(compose_move_req_t));
	payload->x = x;
	payload->y = y;
	payload->z = cli->pos.z;
	compose_request_t* req = compose_client_create_request(COMPOSE_REQ_MOVE, payload);
	int r = compose_client_send_request(cli, req);
	compose_client_release_request(req);
	free(payload);
	return r;
}

int compose_client_layer(compose_client_t* cli, int z) {
	compose_move_req_t* payload = malloc(sizeof(compose_move_req_t));
	payload->x = cli->pos.x;
	payload->y = cli->pos.y;
	payload->z = z;
	compose_request_t* req = compose_client_create_request(COMPOSE_REQ_MOVE, payload);
	int r = compose_client_send_request(cli, req);
	compose_client_release_request(req);
	free(payload);
	return r;
}

int compose_client_resize(compose_client_t* cli, size_t w, size_t h) {
	compose_resize_req_t* payload = malloc(sizeof(compose_resize_req_t));
	payload->w = w;
	payload->h = h;
	compose_request_t* req = compose_client_create_request(COMPOSE_REQ_RESIZE, payload);
	int r = compose_client_send_request(cli, req);
	compose_client_release_request(req);
	free(payload);
	return r;
}

