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

typedef struct {
	int x;
	int y;
	int z;
} position_t;

typedef struct {
	size_t w;
	size_t h;
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

compose_client_t* compose_create_client(int sock);

compose_server_t* compose_sv_create(const char* sock);
compose_client_t* compose_sv_accept(compose_server_t* srv);
void              compose_sv_close(compose_server_t* srv, compose_client_t* cli);
void              compose_sv_tick(compose_server_t* srv);
compose_client_t* compose_sv_get_client(compose_server_t* srv, id_t id);

compose_client_t* compose_cl_connect(const char* sock);
void              compose_cl_disconnect(compose_client_t* client);
id_t              compose_cl_create_window(compose_client_t* client, id_t par, int x, int y, size_t w, size_t h, int flags);
int               compose_cl_move(compose_client_t* cli, id_t win, int x, int y);
int               compose_cl_layer(compose_client_t* cli, id_t win, int z);
int               compose_cl_resize(compose_client_t* cli, id_t win, size_t w, size_t h);

void              compose_sv_move(compose_window_t* win, int x, int y, int z);
void              compose_sv_resize(compose_window_t* win, size_t w, size_t h);
compose_window_t* compose_sv_create_window(compose_server_t* srv, compose_client_t* client, compose_window_t* par, id_t win, int x, int y, size_t w, size_t h, int flags);
compose_window_t* compose_sv_get_window(compose_server_t* srv, id_t win);
void              compose_sv_redraw(compose_server_t* srv);
compose_window_t* compose_sv_get_window_at(compose_server_t* srv, int x, int y);
void              compose_sv_restack(list_t* windows);
void              compose_sv_raise(compose_window_t* win);
void              compose_sv_sunk(compose_window_t* win);
void              compose_sv_focus(compose_window_t* win);

#endif
