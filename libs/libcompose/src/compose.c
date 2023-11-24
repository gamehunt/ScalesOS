#include "compose.h"
#include "events.h"
#include "input/keys.h"
#include "request.h"
#include "fb.h"
#include "stdio.h"
#include "sys/select.h"
#include "sys/un.h"
#include "sys/mman.h"
#include "input/kbd.h"
#include "input/mouse.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "kernel/dev/ps2.h"

static position_t        mouse_pos    = {0};
static buttons_t         last_buttons = {0};
static compose_window_t* input_focus  = NULL;
static uint8_t           kbd_mods     = 0;

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

	compose_client_t* cli = compose_create_client(sockfd);

	while(1) {
		compose_event_t* ev = compose_cl_event_poll(cli);
		event_type type = ev->type;
		id_t win = ev->win;
		free(ev);
		if(type == COMPOSE_EVENT_CNN) {
			cli->root = ev->root;
			return cli;
		}
	}
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
	srv->grabs            = list_create();
	srv->remove_queue     = list_create();
	
	int r =	fb_open("/dev/fb", &srv->framebuffer, FB_FLAG_DOUBLEBUFFER);
	if(r < 0) {
		close(srv->socket);
		list_free(srv->clients);
		free(srv);
		return NULL;
	}

	window_properties_t props;
	memset(&props, 0, sizeof(window_properties_t));
	props.w = srv->framebuffer.info.w;
	props.h = srv->framebuffer.info.h;

	srv->root = compose_sv_create_window(srv, NULL, NULL, props);

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

static compose_client_t* __compose_add_client(compose_server_t* srv, int clifd) {
	if(__compose_get_client(srv, clifd) != NULL) {
		return NULL;
	}
	compose_client_t* client = compose_create_client(clifd);
	client->id = ++srv->client_id;
	list_push_back(srv->clients, client);	
	return client;
}

static void __compose_remove_client(compose_server_t* srv, compose_client_t* cli) {
	list_delete_element(srv->clients, cli);
}

static void __compose_handle_key(compose_server_t* srv, keyboard_packet_t* packet) {
	if(input_focus) {
		compose_key_event_t* event = malloc(sizeof(compose_key_event_t));
		event->event.type = COMPOSE_EVENT_KEY;
		event->event.size = sizeof(compose_key_event_t);
		event->event.win  = input_focus->id;
		if(input_focus->root) {
			event->event.root = input_focus->root->id;
		} else {
			event->event.root = input_focus->id;
		}
		memcpy(&event->packet, packet, sizeof(keyboard_packet_t));	

		uint8_t is_up = event->packet.flags & KBD_EVENT_FLAG_UP;
		if(event->packet.scancode == KEY_LEFTSHIFT ||
		   event->packet.scancode == KEY_RIGHTSHIFT) {
			if(is_up) {
				kbd_mods &= ~KBD_MOD_SHIFT;
			} else {
				kbd_mods |= KBD_MOD_SHIFT;
			}
		} else if(event->packet.scancode == KEY_LEFTALT ||
		   event->packet.scancode == KEY_RIGHTALT) {
			if(is_up) {
				kbd_mods &= ~KBD_MOD_ALT;
			} else {
				kbd_mods |= KBD_MOD_ALT;
			}
		} else if(event->packet.scancode == KEY_LEFTCTRL ||
		   event->packet.scancode == KEY_RIGHTCTRL) {
			if(is_up) {
				kbd_mods &= ~KBD_MOD_CTRL;
			} else {
				kbd_mods |= KBD_MOD_CTRL;
			}
		} else if(event->packet.scancode == KEY_CAPSLOCK) {
			if(kbd_mods & KBD_MOD_CAPS) {
				kbd_mods &= ~KBD_MOD_CAPS;
			} else {
				kbd_mods |= KBD_MOD_CAPS;
			}
		}

		if(!(event->packet.flags & KBD_EVENT_FLAG_EXT)) {
			event->translated = input_kbd_translate(event->packet.scancode, kbd_mods);
		}
		event->modifiers  = kbd_mods;
		compose_sv_event_propagate(srv, input_focus, event);
		free(event);
	}
}

static void __compose_handle_mouse(compose_server_t* srv, mouse_packet_t* packet) {
	__compose_update_mouse(srv, packet->dx, packet->dy);
	compose_mouse_event_t* event = malloc(sizeof(compose_mouse_event_t));
	event->event.type = COMPOSE_EVENT_MOUSE;
	event->event.size = sizeof(compose_mouse_event_t);
	compose_window_t* win = compose_sv_get_window_at(srv, mouse_pos.x, mouse_pos.y);
	event->event.win  = win->id;
	event->event.root = srv->root->id;
	memcpy(&((compose_mouse_event_t*)event)->packet, packet, sizeof(mouse_packet_t));	
	((compose_mouse_event_t*)event)->abs_x = mouse_pos.x;
	((compose_mouse_event_t*)event)->abs_y = mouse_pos.y;
	compose_sv_translate_local(win, mouse_pos.x, mouse_pos.y, 
			&((compose_mouse_event_t*)event)->x,
			&((compose_mouse_event_t*)event)->y);
	compose_sv_event_propagate(srv, win, event);

	if(((mouse_packet_t*)packet)->buttons != last_buttons) {
		event->event.type = COMPOSE_EVENT_BUTTON;
		compose_sv_event_propagate(srv, win, event);
	}

	last_buttons = ((mouse_packet_t*)packet)->buttons;
	free(event);
}

void compose_sv_focus(compose_server_t* srv, compose_window_t* win) {
	if(input_focus) {
		compose_unfocus_event_t* ev = malloc(sizeof(compose_unfocus_event_t));
		ev->event.type = COMPOSE_EVENT_UNFOCUS;
		ev->event.size = sizeof(compose_unfocus_event_t);
		if(input_focus->root) {
			ev->event.root = input_focus->root->id;
		} else {
			ev->event.root = 1;
		}
		ev->event.win = input_focus->id;
		compose_sv_event_propagate(srv, input_focus, ev);
		free(ev);
	}

	if(!win) {
		win = srv->root;
	}

	input_focus = win;

	if(win) {
		compose_focus_event_t* ev = malloc(sizeof(compose_focus_event_t));
		ev->event.type = COMPOSE_EVENT_FOCUS;
		ev->event.size = sizeof(compose_focus_event_t);
		if(input_focus->root) {
			ev->event.root = input_focus->root->id;
		} else {
			ev->event.root = 1;
		}
		ev->event.win = input_focus->id;
		compose_sv_event_propagate(srv, win, ev);
		free(ev);
	}
}

void compose_sv_tick(compose_server_t* srv) {
	fd_set rset;
	FD_ZERO(&rset);

	int n = srv->socket;
	FD_SET(srv->socket, &rset);

	for(int i = 0; i < COMPOSE_DEVICE_AMOUNT; i++) {
		if(n < srv->devices[i]) {
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
			compose_client_t* cli = __compose_add_client(srv, clifd);	
			if(!cli) {
				continue;
			}
			compose_event_t event;
			event.type = COMPOSE_EVENT_CNN;
			event.size = sizeof(compose_event_t);
			event.root = srv->root->id;
			compose_sv_event_send(cli, &event);
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

	while(srv->remove_queue->size > 0) {
		compose_client_t* to_remove = list_pop_back(srv->remove_queue);
		compose_sv_disconnect(srv, to_remove);
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
						e = read(srv->devices[i], &raw_data, 2);
						if(e <= 0) {
							break;
						}
						packet = input_kbd_create_packet((int) raw_data);
						__compose_handle_key(srv, packet);
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

id_t compose_cl_create_window(compose_client_t* client, id_t par, window_properties_t props) {
	compose_win_req_t* req = malloc(sizeof(compose_win_req_t));

	req->req.type = COMPOSE_REQ_NEWWIN;
	req->req.size = sizeof(compose_win_req_t);

	req->parent   = par;
	req->props    = props;

	compose_cl_send_request(client, req);
	free(req);

	while(1) {
		compose_event_t* ev = compose_cl_event_poll(client);
		event_type type = ev->type;
		id_t win = ev->win;
		free(ev);
		if(type == COMPOSE_EVENT_WIN) {
			return win;
		}
	}
}

int compose_cl_evmask(compose_client_t* cli, id_t win, event_mask_t mask) {
	compose_evmask_req_t* payload = malloc(sizeof(compose_evmask_req_t));
	payload->req.type = COMPOSE_REQ_EVMASK;
	payload->req.size = sizeof(compose_evmask_req_t);
	payload->win = win;
	payload->mask = mask;
	int r = compose_cl_send_request(cli, payload);
	free(payload);
	return r;
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
		ev->event.win = win->id;
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

	uint32_t wb = w + 2 * win->sizes.b;
	uint32_t hb = h + 2 * win->sizes.b;
	size_t   bufsz = wb * hb * 4;
	
	win->ctx.info.memsz = bufsz;
	win->ctx.info.w = wb;
	win->ctx.info.h = hb;

	ftruncate(win->ctx_buff_map, bufsz);

	if(win->client) {
		sizes_t new_size = win->sizes;

		compose_resize_event_t* ev = malloc(sizeof(compose_resize_event_t));
		ev->event.type = COMPOSE_EVENT_RESIZE;
		ev->event.size = sizeof(compose_resize_event_t);
		ev->event.win = win->id;
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
		compose_window_t* par, window_properties_t props) {

	compose_window_t* win = calloc(1, sizeof(compose_window_t));

	win->id       	= ++srv->window_id;
	win->parent   	= par;
	win->root     	= srv->root;
	win->client   	= client;
	win->pos.x    	= props.x;
	win->pos.y    	= props.y;
	win->pos.z    	= 0;
	win->sizes.w  	= props.w;
	win->sizes.h  	= props.h;
	win->sizes.b  	= props.border_width;
	win->children 	= list_create();
	win->flags    	= props.flags;
	win->event_mask = props.event_mask;

	uint32_t wb = props.w + 2 * props.border_width;
	uint32_t wh = props.h + 2 * props.border_width;

	size_t buffer_size = wb * wh * 4;

	char key[64];
	itoa(win->id, key, 10);

	win->ctx_map = shm_open(key, O_WRONLY | O_CREAT, 0);
	if(win->ctx_map > 0) {
		ftruncate(win->ctx_map , sizeof(fb_t));
		void* mem = mmap(NULL, sizeof(fb_t), PROT_WRITE, MAP_PRIVATE, win->ctx_map, 0);
		if((int32_t) mem != MAP_FAILED) {
			memcpy(mem, &win->ctx, sizeof(fb_t));
		}
	}

	strcpy(key, "b");

	win->ctx_buff_map = shm_open(key, O_RDWR | O_CREAT, 0);
	if(win->ctx_buff_map > 0) {
		ftruncate(win->ctx_buff_map , buffer_size);
		void* buffer = mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, win->ctx_buff_map, 0);
		if((int32_t) buffer != MAP_FAILED) {
			fb_open_mem(buffer, buffer_size, wb, wh, &win->ctx, 0);
		}
	}

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
		compose_win_event_t* ev = calloc(1, sizeof(compose_win_event_t));
		ev->event.type = COMPOSE_EVENT_WIN;
		ev->event.size = sizeof(compose_win_event_t);
		ev->event.win  = win->id;
		ev->event.root = win->root->id;
		compose_sv_event_send(client, ev);
		compose_sv_event_propagate(srv, win, ev);
		free(ev);
	}

	compose_sv_focus(srv, win);

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

static void __compose_draw_window(compose_server_t* srv, compose_window_t* win, position_t* off) {
	position_t pos = win->pos;
	if(off) {
		pos.x += off->x;
		pos.y += off->y;
	}
	fb_bitmap(&srv->framebuffer, pos.x, pos.y, win->sizes.w + 2 * win->sizes.b, win->sizes.h + 2 * win->sizes.b, 32, win->ctx.mem);
	off = &pos;
	for(size_t i = 0; i < win->children->size; i++) {
		__compose_draw_window(srv, win->children->data[i], off);
	}
}

void compose_sv_redraw(compose_server_t* srv) {
	compose_sv_restack(srv->root->children);
	fb_fill(&srv->framebuffer, 0xFF000000);	
	__compose_draw_window(srv, srv->root, NULL);
	__compose_draw_mouse(srv);
	fb_flush(&srv->framebuffer);
}

compose_window_t* compose_sv_get_window_at(compose_server_t* srv, int x, int y) {
	if(!srv->windows->size) {
		return NULL;
	}
	for(int32_t i = srv->windows->size - 1; i >= 0; i--) {
		compose_window_t* win = srv->windows->data[i];
		int sx, sy;
		compose_sv_translate_abs(win, &sx, &sy, 0, 0);
		if(x >= sx && 
		   y >= sy && 
		   x <= sx + win->sizes.w + win->sizes.b && 
		   y <= sy + win->sizes.h + win->sizes.b) {
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

	compose_sv_move(win, -1, -1, highest + 1);
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

	compose_sv_move(win, -1, -1, lowest - 1);
}

void compose_sv_translate_local(compose_window_t* win, int sx, int sy, int* x, int* y) {
	int ox = win->pos.x;
	int oy = win->pos.y;

	while(win->parent) {
		ox += win->parent->pos.x;
		oy += win->parent->pos.y;
		win = win->parent;
	}

	*x = sx - ox;
	*y = sy - oy;
}

void compose_sv_translate_abs(compose_window_t* win, int* sx, int* sy, int x, int y) {
	int ox = win->pos.x;
	int oy = win->pos.y;

	while(win->parent) {
		ox += win->parent->pos.x;
		oy += win->parent->pos.y;
		win = win->parent;
	}

	*sx = x + ox;
	*sy = y + oy;
}

int compose_cl_grab(compose_client_t* cli, id_t win, grab_type type) {
	compose_grab_req_t* payload = malloc(sizeof(compose_grab_req_t));
	payload->req.type = COMPOSE_REQ_GRAB;
	payload->req.size = sizeof(compose_grab_req_t);
	payload->win  = win;
	payload->type = type;
	int r = compose_cl_send_request(cli, payload);
	free(payload);
	return r;
}

int compose_cl_focus(compose_client_t* cli, id_t win) {
	compose_focus_req_t* payload = malloc(sizeof(compose_focus_req_t));
	payload->req.type = COMPOSE_REQ_FOCUS;
	payload->req.size = sizeof(compose_focus_req_t);
	payload->win  = win;
	int r = compose_cl_send_request(cli, payload);
	free(payload);
	return r;
}

int compose_cl_unfocus(compose_client_t* cli, id_t win) {
	compose_focus_req_t* payload = malloc(sizeof(compose_focus_req_t));
	payload->req.type = COMPOSE_REQ_FOCUS;
	payload->req.size = sizeof(compose_focus_req_t);
	payload->win = -1;
	int r = compose_cl_send_request(cli, payload);
	free(payload);
	return r;
}

uint8_t compose_sv_is_child(compose_window_t* win, compose_window_t* par) {
	while(win) {
		if(win->parent == par) {
			return 1;
		}
		win = win->parent;
	}

	return 0;
}

void compose_cl_disconnect(compose_client_t* client) {
	compose_disconnect_req_t* payload = malloc(sizeof(compose_disconnect_req_t));
	payload->req.type = COMPOSE_REQ_DISCONNECT;
	payload->req.size = sizeof(compose_disconnect_req_t);
	int r = compose_cl_send_request(client, payload);
	free(payload);
	close(client->socket);
}

void compose_sv_remove_window(compose_server_t* srv, compose_window_t* win, int notify_parent) {
	if(win == input_focus) {
		input_focus = NULL;
	}

	for(size_t i = 0; i < win->children->size; i++) {
		compose_sv_remove_window(srv, win->children->data[i], 0);
	}
	list_free(win->children);

	if(notify_parent && win->parent) {
		list_delete_element(win->parent->children, win);
	}

	list_delete_element(srv->windows, win);

	list_t* active_grabs = list_create();
	for(size_t i = 0; i < srv->grabs->size; i++) {
		grab_t* grab = srv->grabs->data[i];
		if(grab->window == win) {
			list_push_back(active_grabs, grab);
		}
	}

	for(size_t i = 0; i < active_grabs->size; i++) {
		grab_t* grab = active_grabs->data[i];
		list_delete_element(srv->grabs, grab);
		free(grab);
	}
	list_free(active_grabs);

	close(win->ctx_map);
	close(win->ctx_buff_map);
	munmap(win->ctx.mem, win->ctx.info.memsz);

	free(win);
}

void compose_sv_disconnect(compose_server_t* srv, compose_client_t* client) {	
	__compose_remove_client(srv, client);

	list_t* child_windows = list_create();
	for(size_t i = 0; i < srv->windows->size; i++) {
		compose_window_t* win = srv->windows->data[i];
		if(win->client == client && (!win->parent || win->parent == srv->root)) {
			list_push_back(child_windows, win);
		}
	}

	for(size_t i = 0; i < child_windows->size; i++) {
		compose_window_t* win = child_windows->data[i];
		compose_sv_remove_window(srv, win, 1);
	}
	list_free(child_windows);

	list_t* active_grabs = list_create();
	for(size_t i = 0; i < srv->grabs->size; i++) {
		grab_t* grab = srv->grabs->data[i];
		if(grab->client == client) {
			list_push_back(active_grabs, grab);
		}
	}

	for(size_t i = 0; i < active_grabs->size; i++) {
		grab_t* grab = active_grabs->data[i];
		list_delete_element(srv->grabs, grab);
		free(grab);
	}
	list_free(active_grabs);

	close(client->socket);
	free(client);
}


void compose_sv_send_props(compose_client_t* cli, compose_window_t* win) {
	compose_props_event_t* ev = malloc(sizeof(compose_props_event_t));
	ev->event.type = COMPOSE_EVENT_PROPS;
	ev->event.size = sizeof(compose_props_event_t);
	ev->event.win  = win->id;
	if(win->root) {
		ev->event.root = win->root->id;
	} else {
		ev->event.root = win->id;
	}
	ev->props.x = win->pos.x;
	ev->props.y = win->pos.y;
	ev->props.w = win->sizes.w;
	ev->props.h = win->sizes.h;
	ev->props.flags = win->flags;
	ev->props.border_width = win->sizes.b;
	ev->props.event_mask   = win->event_mask;
	compose_sv_event_send(cli, ev);
	free(ev);
}

window_properties_t compose_cl_get_properties(compose_client_t* cli, id_t win) {
	compose_props_req_t* payload = malloc(sizeof(compose_props_req_t));
	payload->req.type = COMPOSE_REQ_PROPS;
	payload->req.size = sizeof(compose_props_req_t);
	payload->win      = win;
	int r = compose_cl_send_request(cli, payload);
	free(payload);

	while(1) {
		compose_event_t* ev = compose_cl_event_poll(cli);
		if(ev) { 
			window_properties_t props = {0};
			int f = 0;
			if(ev->type == COMPOSE_EVENT_PROPS) {
				props = ((compose_props_event_t*) ev)->props;
				f = 1;
			}
			free(ev);
			if(f) {
				return props;
			}
		}
	}
}

fb_t* compose_cl_draw_ctx(id_t win) {
	char key[64];
	itoa(win, key, 10);

	int ctx = shm_open(key, O_RDONLY, 0);
	if(ctx < 0) {
		return NULL;
	}

	fb_t* buff = mmap(NULL, sizeof(fb_t), PROT_READ, MAP_PRIVATE, ctx, 0);
	if((int32_t) buff == MAP_FAILED) {
		return NULL;
	}

	strcat(key, "b");

	ctx = shm_open(key, O_RDWR, 0);
	if(ctx < 0) {
		return NULL;
	}
	buff->mem = mmap(NULL, buff->info.memsz, PROT_READ | PROT_WRITE, MAP_SHARED, ctx, 0);

	return buff;
}
