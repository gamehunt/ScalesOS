#ifndef __LIB_COMPOSE_H
#define __LIB_COMPOSE_H

#include "fb.h"
#include "types/list.h"
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef uint32_t id_t;
typedef uint32_t flags_t;

#define COMPOSE_DEVICE_KBD    0
#define COMPOSE_DEVICE_MOUSE  1

#define COMPOSE_DEVICE_AMOUNT 2

#define COMPOSE_WIN_FLAGS_MOVABLE   (1 << 0)
#define COMPOSE_WIN_FLAGS_RESIZABLE (1 << 1)

#define COMPOSE_BORDER_NONE      0
#define COMPOSE_BORDER_UP        1
#define COMPOSE_BORDER_DOWN      2
#define COMPOSE_BORDER_LEFT      3
#define COMPOSE_BORDER_RIGHT     4
#define COMPOSE_BORDER_CORNER_LU 5
#define COMPOSE_BORDER_CORNER_LB 6
#define COMPOSE_BORDER_CORNER_RU 7
#define COMPOSE_BORDER_CORNER_RB 8

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
	pid_t      pid;
	id_t       win_id;
	int        socket;
} compose_client_t;

typedef struct window {
	id_t              id;
	compose_client_t* client;
	struct window*    parent;
	fb_t       		  ctx;
	flags_t    		  flags;
	position_t 		  pos;
	int               layer;
	sizes_t    		  sizes;
	list_t*    		  children;
} compose_window_t;

typedef struct {
	id_t       client_id;
	int  	   socket;
	fb_t 	   framebuffer;
	int  	   devices[COMPOSE_DEVICE_AMOUNT];
	list_t*    clients;
	compose_window_t* root;
	list_t*    windows;
} compose_server_t;

typedef struct {
	id_t    id;
	int     x, y;
	size_t  w, h;
	int     flags;
	size_t  border_width;
} window_properties_t;

compose_client_t* compose_create_client(int sock);

compose_server_t* compose_sv_create(const char* sock);
compose_client_t* compose_sv_accept(compose_server_t* srv);
void              compose_sv_close(compose_server_t* srv, compose_client_t* cli);
void              compose_sv_tick(compose_server_t* srv);
compose_client_t* compose_sv_get_client(compose_server_t* srv, id_t id);

compose_client_t* compose_cl_connect(const char* sock);
void              compose_cl_disconnect(compose_client_t* client);
id_t              compose_cl_create_window(compose_client_t* client, id_t par, window_properties_t props);
int               compose_cl_move(compose_client_t* cli, id_t win, int x, int y);
int               compose_cl_layer(compose_client_t* cli, id_t win, int z);
int               compose_cl_resize(compose_client_t* cli, id_t win, size_t w, size_t h);

void              compose_sv_move(compose_window_t* win, int x, int y, int z);
void              compose_sv_resize(compose_window_t* win, size_t w, size_t h);
compose_window_t* compose_sv_create_window(compose_server_t* srv, compose_client_t* client, compose_window_t* par, window_properties_t props);
compose_window_t* compose_sv_get_window(compose_server_t* srv, id_t win);
void              compose_sv_redraw(compose_server_t* srv);
compose_window_t* compose_sv_get_window_at(compose_server_t* srv, int x, int y);
uint8_t           compose_sv_is_at_border(compose_window_t* win, int x, int y);
void              compose_sv_restack(list_t* windows);
void              compose_sv_raise(compose_window_t* win);
void              compose_sv_sunk(compose_window_t* win);
void              compose_sv_focus(compose_window_t* win);
void              compose_sv_translate(compose_window_t* win, int sx, int sy, int* x, int* y);

#endif
