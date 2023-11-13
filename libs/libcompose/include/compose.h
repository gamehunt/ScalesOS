#ifndef __LIB_COMPOSE_H
#define __LIB_COMPOSE_H

#include "fb.h"
#include "types/list.h"
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef uint32_t id_t;
typedef uint32_t flags_t;
typedef uint64_t event_mask_t;
typedef event_mask_t grab_type;

#define COMPOSE_DEVICE_KBD    0
#define COMPOSE_DEVICE_MOUSE  1

#define COMPOSE_DEVICE_AMOUNT 2

#define COMPOSE_WIN_FLAGS_MOVABLE   (1 << 0)
#define COMPOSE_WIN_FLAGS_RESIZABLE (1 << 1)

typedef struct {
	int x;
	int y;
	int z;
} position_t;

typedef struct {
	size_t w;
	size_t h;
	size_t b;
} sizes_t;

typedef struct {
	id_t       id;
	id_t       root;
	pid_t      pid;
	int        socket;
} compose_client_t;

typedef struct window {
	id_t              id;
	compose_client_t* client;
	struct window*    parent;
	struct window*    root;
	fb_t       		  ctx;
	flags_t    		  flags;
	position_t 		  pos;
	int               layer;
	sizes_t    		  sizes;
	list_t*    		  children;
	event_mask_t      event_mask;
} compose_window_t;

typedef struct {
	grab_type         type;
	compose_client_t* client;
	compose_window_t* window;
} grab_t;

typedef struct {
	id_t       		  client_id;
	id_t       		  window_id;
	int  	   		  socket;
	fb_t 	   		  framebuffer;
	int  	   		  devices[COMPOSE_DEVICE_AMOUNT];
	compose_window_t* root;
	list_t*    		  clients;
	list_t*           remove_queue;
	list_t*           windows;
	list_t*           grabs;
} compose_server_t;

typedef struct {
	int     	 x, y;
	size_t  	 w, h;
	int     	 flags;
	size_t  	 border_width;
	event_mask_t event_mask;
} window_properties_t;

compose_client_t* compose_create_client(int sock);

compose_server_t* compose_sv_create(const char* sock);
compose_client_t* compose_sv_accept(compose_server_t* srv);
void              compose_sv_tick(compose_server_t* srv);
compose_client_t* compose_sv_get_client(compose_server_t* srv, id_t id);
void              compose_sv_disconnect(compose_server_t* srv, compose_client_t* cli);

compose_client_t* compose_cl_connect(const char* sock);
void              compose_cl_disconnect(compose_client_t* client);
id_t              compose_cl_create_window(compose_client_t* client, id_t par, window_properties_t props);
int               compose_cl_move(compose_client_t* cli, id_t win, int x, int y);
int               compose_cl_layer(compose_client_t* cli, id_t win, int z);
int               compose_cl_resize(compose_client_t* cli, id_t win, size_t w, size_t h);
int               compose_cl_evmask(compose_client_t* cli, id_t win, event_mask_t mask);
int               compose_cl_grab(compose_client_t* cli, id_t win, grab_type type);
int               compose_cl_focus(compose_client_t* cli, id_t win);
int               compose_cl_unfocus(compose_client_t* cli, id_t win);
window_properties_t compose_cl_get_properties(compose_client_t* cli, id_t win);

void              compose_sv_move(compose_window_t* win, int x, int y, int z);
void              compose_sv_resize(compose_window_t* win, size_t w, size_t h);
compose_window_t* compose_sv_create_window(compose_server_t* srv, compose_client_t* client, compose_window_t* par, window_properties_t props);
compose_window_t* compose_sv_get_window(compose_server_t* srv, id_t win);
void              compose_sv_redraw(compose_server_t* srv);
compose_window_t* compose_sv_get_window_at(compose_server_t* srv, int x, int y);
void              compose_sv_restack(list_t* windows);
void              compose_sv_raise(compose_window_t* win);
void              compose_sv_sunk(compose_window_t* win);
void              compose_sv_focus(compose_server_t* srv, compose_window_t* win);
void              compose_sv_translate_local(compose_window_t* win, int sx, int sy, int* x, int* y);
void              compose_sv_translate_abs(compose_window_t* win, int* sx, int* sy, int x, int y);
uint8_t           compose_sv_is_child(compose_window_t* win, compose_window_t* par);
void 			  compose_sv_remove_window(compose_server_t* srv, compose_window_t* win, int notify_parent);
void              compose_sv_send_props(compose_client_t* cli, compose_window_t* win);

#endif
