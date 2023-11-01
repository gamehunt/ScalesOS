#include "compose.h"
#include "events.h"
#include "fb.h"
#include "kernel/dev/ps2.h"
#include "request.h"
#include "sys/select.h"
#include "sys/time.h"
#include "sys/mman.h"
#include "sys/un.h"
#include "input/kbd.h"
#include "input/mouse.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

static position_t        mouse_pos   = {0};
static compose_window_t* input_focus = NULL;

#define MOVING   (1 << 0)
#define RESIZING (1 << 1)

static uint8_t           g_flags = 0;

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
}

static void __compose_draw_mouse(compose_server_t* srv) {
	fb_filled_rect(&srv->framebuffer, mouse_pos.x, mouse_pos.y, 4, 4, 0xFF00FF00, 0xFF00FF00);
}

compose_client_t* compose_cl_connect(const char* sock) {
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
	srv->windows          = list_create();
	
	int r =	fb_open("/dev/fb", &srv->framebuffer, FB_FLAG_DOUBLEBUFFER);
	if(r < 0) {
		close(srv->socket);
		list_free(srv->clients);
		free(srv);
		return NULL;
	}

	srv->root = compose_sv_create_window(srv, NULL, NULL, 0, 0, 0, srv->framebuffer.info.w, srv->framebuffer.info.h, 0);

	srv->devices[0] = open("/dev/kbd", O_RDONLY | O_NOBLOCK);
	srv->devices[1] = open("/dev/mouse", O_RDONLY | O_NOBLOCK);
	
	listen(sockfd, 10);

	return srv;
}

compose_client_t* compose_create_client(int sock) {
	compose_client_t* cli = calloc(1, sizeof(compose_client_t));
	cli->socket = sock;
	cli->win_id = 1;
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
	compose_client_t* client = compose_create_client(clifd);
	client->id = ++srv->client_id;
	list_push_back(srv->clients, client);	
}

static void __compose_remove_client(compose_server_t* srv, compose_client_t* cli) {
	list_delete_element(srv->clients, cli);
}

void compose_sv_close(compose_server_t* srv, compose_client_t* cli) {
	__compose_remove_client(srv, cli);
	close(cli->socket);
	free(cli);
}

static void __compose_handle_key(compose_server_t* srv, keyboard_packet_t* packet) {

}

static void __compose_handle_mouse(compose_server_t* srv, mouse_packet_t* packet) {
	__compose_update_mouse(srv, packet->dx, packet->dy);

	if(g_flags & MOVING) {
		compose_sv_move(input_focus, input_focus->pos.x + packet->dx, input_focus->pos.y - packet->dy, -1);
	}

	if((packet->buttons & MOUSE_BUTTON_LEFT)) {
		if(g_flags & MOVING) {
			return;
		}
		compose_window_t* win = compose_sv_get_window_at(srv, mouse_pos.x, mouse_pos.y);
		if(win) {
			compose_sv_focus(win);
			if(win->flags & COMPOSE_WIN_FLAGS_MOVABLE) {
				g_flags |= MOVING;
			}
		}
	} else {
		g_flags &= ~MOVING;
	}
}

void compose_sv_focus(compose_window_t* win) {
	input_focus = win;
	if(win) {
		compose_sv_raise(win);
	}
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
						packet = input_kbd_create_packet((int) raw_data);
						__compose_handle_key(srv, packet);
						if(input_focus && input_focus->client) {
							event = malloc(sizeof(compose_key_event_t));
							event->type = COMPOSE_EVENT_KEY;
							event->size = sizeof(compose_key_event_t);
							((compose_key_event_t*)event)->win  = input_focus->id;
							memcpy(&((compose_key_event_t*)event)->packet, packet, sizeof(keyboard_packet_t));	
							compose_sv_event_send(input_focus->client, event);
							free(event);
						}
						free(packet);
						break;
					case COMPOSE_DEVICE_MOUSE:
						raw_data = malloc(sizeof(ps_mouse_packet_t));
						e = read(srv->devices[i], raw_data, sizeof(ps_mouse_packet_t));
						if(e <= 0) {
							free(raw_data);
							break;
						}
						packet = input_mouse_create_packet(raw_data);
						free(raw_data);
						if(!packet) {
							break;
						}
						__compose_handle_mouse(srv, packet);
						if(input_focus && input_focus->client) {
							event = malloc(sizeof(compose_mouse_event_t));
							event->type = COMPOSE_EVENT_MOUSE;
							event->size = sizeof(compose_mouse_event_t);
							((compose_mouse_event_t*)event)->win  = input_focus->id;
							memcpy(&((compose_mouse_event_t*)event)->packet, packet, sizeof(mouse_packet_t));	
							compose_sv_event_send(input_focus->client, event);
							free(event);
						}
						free(packet);
						break;
				}
				if(e <= 0) {
					break;
				}
			}
		}
	}

	compose_sv_redraw(srv);
}

int compose_cl_move(compose_client_t* cli, id_t win, int x, int y) {
	compose_move_req_t* payload = malloc(sizeof(compose_move_req_t));
	payload->req.type = COMPOSE_REQ_MOVE;
	payload->req.size = sizeof(compose_move_req_t);
	payload->win = win;
	payload->x = x;
	payload->y = y;
	payload->z = -1;
	int r = compose_cl_send_request(cli, payload);
	free(payload);
	return r;
}

int compose_cl_layer(compose_client_t* cli, id_t win, int z) {
	compose_move_req_t* payload = malloc(sizeof(compose_move_req_t));
	payload->req.type = COMPOSE_REQ_MOVE;
	payload->req.size = sizeof(compose_move_req_t);
	payload->win = win;
	payload->x = -1;
	payload->y = -1;
	payload->z = z;
	int r = compose_cl_send_request(cli, payload);
	free(payload);
	return r;
}

int compose_cl_resize(compose_client_t* cli, id_t win, size_t w, size_t h) {
	compose_resize_req_t* payload = malloc(sizeof(compose_resize_req_t));
	payload->req.type = COMPOSE_REQ_RESIZE;
	payload->req.size = sizeof(compose_resize_req_t);
	payload->win = win;
	payload->w = w;
	payload->h = h;
	int r = compose_cl_send_request(cli, payload);
	free(payload);
	return r;
}

id_t compose_cl_create_window(compose_client_t* client, id_t par, int x, int y, size_t w, size_t h, int flags) {
	compose_win_req_t* req = malloc(sizeof(compose_win_req_t));

	req->req.type = COMPOSE_REQ_NEWWIN;
	req->req.size = sizeof(compose_win_req_t);

	req->id    = ++client->win_id;
	req->par   = par;
	req->x     = x;
	req->y     = y;
	req->w     = w;
	req->h     = h;
	req->flags = flags;

	compose_cl_send_request(client, req);

	id_t id = req->id;
	free(req);

	return id;
}

void compose_sv_move(compose_window_t* win, int x, int y, int z) {
	position_t old_pos = win->pos;

	if(x >= 0) {
		win->pos.x = x;
	}

	if(y >= 0) {
		win->pos.y = y;
	}

	if(z >= 0) {
		win->pos.z = z;
		if(win->parent) {
			compose_sv_restack(win->parent->children);
		}
	}

	if(win->client) {
		position_t new_pos = win->pos;

		compose_move_event_t* ev = malloc(sizeof(compose_move_event_t));
		ev->event.type = COMPOSE_EVENT_MOVE;
		ev->event.size = sizeof(compose_move_event_t);
		ev->win = win->id;
		ev->old_pos = old_pos;
		ev->new_pos = new_pos;

		compose_sv_event_send(win->client, ev);
		free(ev);
	}
}

void compose_sv_resize(compose_window_t* win, size_t w, size_t h) {
	sizes_t old_size = win->sizes;

	win->sizes.w = w;
	win->sizes.h = h;

	fb_close(&win->ctx);

	size_t bufsz = w * h * 4;
	void* buf = malloc(bufsz);
	fb_open_mem(buf, bufsz, w, h, &win->ctx, 0);

	if(win->client) {
		sizes_t new_size = win->sizes;

		compose_resize_event_t* ev = malloc(sizeof(compose_resize_event_t));
		ev->event.type = COMPOSE_EVENT_RESIZE;
		ev->event.size = sizeof(compose_resize_event_t);
		ev->win = win->id;
		ev->old_size = old_size;
		ev->new_size = new_size;

		compose_sv_event_send(win->client, ev);
		free(ev);
	}
}

compose_window_t* compose_sv_get_window(compose_server_t* srv, id_t win) {
	for(size_t i = 0; i < srv->windows->size; i++) {
		compose_window_t* w = srv->windows->data[i];
		if(w->id == win) {
			return w;
		}
	}
	return NULL;
}

static uint8_t __compose_sort_windows_list(compose_window_t* a, compose_window_t* b) {
	if(a->layer == b->layer) {
		return a->pos.z > b->pos.z;
	}
	return a->layer > b->layer;
}

compose_window_t* compose_sv_create_window(compose_server_t* srv, compose_client_t* client, 
		compose_window_t* par, id_t win_id, int x, int y, size_t w, size_t h, int flags) {

	compose_window_t* win = calloc(1, sizeof(compose_window_t));

	win->id       = win_id;
	win->parent   = par;
	win->client   = client;
	win->pos.x    = x;
	win->pos.y    = y;
	win->pos.z    = 0;
	win->sizes.w  = w;
	win->sizes.h  = h;
	win->children = list_create();
	win->flags    = flags;

	size_t buffer_size = w * h * 4;
	void*  buffer = calloc(1, buffer_size);

	fb_open_mem(buffer, buffer_size, w, h, &win->ctx, 0);

	list_push_back(srv->windows, win);
	if(par) {
		win->layer = par->layer + 1;
		list_push_back(par->children, win);
		compose_sv_restack(par->children);
	} else {
		win->layer = 0;
	}
	list_sort(srv->windows, __compose_sort_windows_list);

	if(client) {
		compose_win_event_t* ev = malloc(sizeof(compose_win_event_t));
		ev->event.type = COMPOSE_EVENT_WIN;
		ev->event.size = sizeof(compose_win_event_t);
		ev->win = win->id;
		compose_sv_event_send(client, ev);
		free(ev);
	}

	compose_sv_focus(win);

	return win;
}

compose_client_t* compose_sv_get_client(compose_server_t* srv, id_t id) {
	for(size_t i = 0; i < srv->clients->size; i++) {
		compose_client_t* cli = srv->clients->data[i];
		if(cli->id == id) {
			return cli;
		}
	}
	return NULL;
}

static void __compose_draw_window(compose_server_t* srv, compose_window_t* win) {
	fb_bitmap(&srv->framebuffer, win->pos.x, win->pos.y, win->sizes.w, win->sizes.h, win->ctx.mem);
	for(size_t i = 0; i < win->children->size; i++) {
		__compose_draw_window(srv, win->children->data[i]);
	}
}

void compose_sv_redraw(compose_server_t* srv) {
	compose_sv_restack(srv->root->children);
	fb_fill(&srv->framebuffer, 0xFF000000);	
	__compose_draw_window(srv, srv->root);
	__compose_draw_mouse(srv);
	fb_flush(&srv->framebuffer);
}

compose_window_t* compose_sv_get_window_at(compose_server_t* srv, int x, int y) {
	if(!srv->windows->size) {
		return NULL;
	}
	for(int32_t i = srv->windows->size - 1; i >= 0; i--) {
		compose_window_t* win = srv->windows->data[i];
		if(x >= win->pos.x && y >= win->pos.y && x <= win->pos.x + win->sizes.w && y <= win->pos.y + win->sizes.h) {
			return win;
		}
	}
	return NULL;
}

static uint8_t __compose_sort_windows(compose_window_t* a, compose_window_t* b) {
	return a->pos.z > b->pos.z;
}

void compose_sv_restack(list_t* windows) {
	list_sort(windows, __compose_sort_windows);
	for(size_t i = 0; i < windows->size; i++) {
		compose_window_t* win = windows->data[i];
		compose_sv_restack(win->children);
	}
} 

void compose_sv_raise(compose_window_t* win) {
	if(!win->parent) {
		return;
	}	

	list_t* siblings = win->parent->children;

	compose_window_t* top = list_tail(siblings);
	if(top == win) {
		return;
	}

	int highest = top->pos.z;

	compose_sv_move(win, 0, 0, highest + 1);
}

void compose_sv_sunk(compose_window_t* win) {
	if(!win->parent) {
		return;
	}	

	list_t* siblings = win->parent->children;

	int lowest = ((compose_window_t*) list_head(siblings))->pos.z;
	if(lowest < 0) {
		lowest = 0;
	}

	compose_sv_move(win, 0, 0, lowest - 1);
}
