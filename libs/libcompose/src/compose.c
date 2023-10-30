#include "compose.h"
#include "events.h"
#include "fb.h"
#include "kernel/dev/ps2.h"
#include "request.h"
#include "sys/select.h"
#include "sys/time.h"
#include "sys/un.h"
#include "input/kbd.h"
#include "input/mouse.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

static position_t mouse_pos = {0};

static void __compose_update_mouse(compose_server_t* srv, int dx, int dy) {
	position_t old = mouse_pos;

	mouse_pos.x += dx;
	mouse_pos.y -= dy;

	if(mouse_pos.x < 0) {
		mouse_pos.x = 0;
	} 

	if(mouse_pos.y < 0) {
		mouse_pos.y = 0;
	} 

	if(mouse_pos.x >= srv->framebuffer.info.w) {
		mouse_pos.x = srv->framebuffer.info.w - 4;
	}

	if(mouse_pos.y >= srv->framebuffer.info.h) {
		mouse_pos.y = srv->framebuffer.info.h - 4;
	}

	fb_filled_rect(&srv->framebuffer, old.x, old.y, 4, 4, 0xFF000000, 0xFF000000);
	fb_filled_rect(&srv->framebuffer, mouse_pos.x, mouse_pos.y, 4, 4, 0xFF00FF00, 0xFF00FF00);

	fb_flush(&srv->framebuffer);
}

compose_client_t* compose_connect(const char* sock) {
	int sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if(sockfd < 0) {
		return NULL;
	}

	struct sockaddr_un addr;
	addr.sun_family = AF_LOCAL;
	strncpy(addr.sun_path, sock, sizeof(addr.sun_path));

	if(connect(sockfd, &addr ,sizeof(addr)) < 0) {
		return NULL;
	}

	return compose_create_client(sockfd);
}

compose_server_t* compose_sv_create(const char* sock) {
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
	srv->clients          = list_create();
	
	int r =	fb_open("/dev/fb", &srv->framebuffer);
	if(r < 0) {
		close(srv->socket);
		list_free(srv->clients);
		free(srv);
		return NULL;
	}

	srv->devices[0] = open("/dev/kbd", O_RDONLY | O_NOBLOCK);
	srv->devices[1] = open("/dev/mouse", O_RDONLY | O_NOBLOCK);
	
	listen(sockfd, 10);

	return srv;
}

compose_client_t* compose_create_client(int sock) {
	compose_client_t* cli = calloc(1, sizeof(compose_client_t));
	cli->socket = sock;
	return cli;
}

static compose_client_t* __compose_get_client(compose_server_t* srv, int sock) {
	for(size_t i = 0; i < srv->clients->size; i++) {
		compose_client_t* cli = srv->clients->data[i];
		if(cli->socket == sock) {
			return cli;
		}
	}
	return NULL;
}

static void __compose_add_client(compose_server_t* srv, int clifd) {
	if(__compose_get_client(srv, clifd) != NULL) {
		return;
	}
	list_push_back(srv->clients, compose_create_client(clifd));	
}

static void __compose_remove_client(compose_server_t* srv, compose_client_t* cli) {
	list_delete_element(srv->clients, cli);
}

void compose_sv_close(compose_server_t* srv, compose_client_t* cli) {
	__compose_remove_client(srv, cli);
	close(cli->socket);
	free(cli);
}

void compose_sv_tick(compose_server_t* srv) {
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

	for(size_t i = 0; i < srv->clients->size; i++) {
		compose_client_t* cli = srv->clients->data[i];
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
		while((clifd = accept(srv->socket, NULL, NULL)) >= 0) {
			__compose_add_client(srv, clifd);	
		}
	}

	for(size_t i = 0; i < srv->clients->size; i++) {
		compose_client_t* cli = srv->clients->data[i];
		if(FD_ISSET(cli->socket, &rset)) {
			compose_request_t* req;
			while((req = compose_sv_request_poll(cli))) {
				compose_sv_handle_request(srv, cli, req);
				free(req);
			}
		}
	}

	for(int i = 0; i < COMPOSE_DEVICE_AMOUNT; i++) {
		if(FD_ISSET(srv->devices[i], &rset)) {
			while(1) {
				compose_event_t* event = NULL;
				void* raw_data;
				void* packet;
				int   e = 0;
				switch(i) {
					case COMPOSE_DEVICE_KBD:
						e = read(srv->devices[i], &raw_data, 1);
						if(e <= 0) {
							break;
						}
						event = malloc(sizeof(compose_key_event_t));
						event->type = COMPOSE_EVENT_KEY;
						event->size = sizeof(compose_key_event_t);
						packet = input_kbd_create_packet((int) raw_data);
						memcpy(&((compose_key_event_t*)event)->packet, packet, sizeof(keyboard_packet_t));	
						free(packet);
						break;
					case COMPOSE_DEVICE_MOUSE:
						raw_data = malloc(sizeof(ps_mouse_packet_t));
						e = read(srv->devices[i], raw_data, sizeof(ps_mouse_packet_t));
						if(e <= 0) {
							free(raw_data);
							break;
						}
						event = malloc(sizeof(compose_mouse_event_t));
						event->type = COMPOSE_EVENT_MOUSE;
						event->size = sizeof(compose_mouse_event_t);
						packet = input_mouse_create_packet(raw_data);
						free(raw_data);
						if(!packet) {
							break;
						}
						__compose_update_mouse(srv, ((mouse_packet_t*) packet)->dx, ((mouse_packet_t*) packet)->dy);
						memcpy(&((compose_mouse_event_t*)event)->packet, packet, sizeof(mouse_packet_t));	
						free(packet);
						break;
				}
				if(event) {
					compose_sv_event_send_to_all(srv, event);
					free(event);
				} else {
					break;
				}
			}
		}
	}
}

int compose_cl_move(compose_client_t* cli, int x, int y) {
	compose_move_req_t* payload = malloc(sizeof(compose_move_req_t));
	payload->req.type = COMPOSE_REQ_MOVE;
	payload->req.size = sizeof(compose_move_req_t);
	payload->x = x;
	payload->y = y;
	payload->z = cli->pos.z;
	int r = compose_cl_send_request(cli, payload);
	free(payload);
	return r;
}

int compose_cl_layer(compose_client_t* cli, int z) {
	compose_move_req_t* payload = malloc(sizeof(compose_move_req_t));
	payload->req.type = COMPOSE_REQ_MOVE;
	payload->req.size = sizeof(compose_move_req_t);
	payload->x = cli->pos.x;
	payload->y = cli->pos.y;
	payload->z = z;
	int r = compose_cl_send_request(cli, payload);
	free(payload);
	return r;
}

int compose_cl_resize(compose_client_t* cli, size_t w, size_t h) {
	compose_resize_req_t* payload = malloc(sizeof(compose_resize_req_t));
	payload->req.type = COMPOSE_REQ_RESIZE;
	payload->req.size = sizeof(compose_resize_req_t);
	payload->w = w;
	payload->h = h;
	int r = compose_cl_send_request(cli, payload);
	free(payload);
	return r;
}

void compose_sv_move(compose_client_t* cli, int x, int y, int z) {
	position_t old_pos = cli->pos;

	cli->pos.x = x;
	cli->pos.y = y;
	cli->pos.z = z;

	position_t new_pos = cli->pos;

	compose_move_event_t* ev = malloc(sizeof(compose_move_event_t));
	ev->event.type = COMPOSE_EVENT_MOVE;
	ev->event.size = sizeof(compose_move_event_t);
	ev->old_pos = old_pos;
	ev->new_pos = new_pos;

	compose_sv_event_send(cli, ev);
	free(ev);
}

void compose_sv_resize(compose_client_t* cli, size_t w, size_t h) {
	sizes_t old_size = cli->sizes;

	cli->sizes.w = w;
	cli->sizes.h = h;

	sizes_t new_size = cli->sizes;

	compose_resize_event_t* ev = malloc(sizeof(compose_resize_event_t));
	ev->event.type = COMPOSE_EVENT_RESIZE;
	ev->event.size = sizeof(compose_resize_event_t);
	ev->old_size = old_size;
	ev->new_size = new_size;

	compose_sv_event_send(cli, ev);
	free(ev);
}
