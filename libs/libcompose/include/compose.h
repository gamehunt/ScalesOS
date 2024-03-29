#ifndef __LIB_COMPOSE_H
#define __LIB_COMPOSE_H

#include "fb.h"
#include "types/list.h"
#include <stddef.h>
#include <stdint.h>
#include <sys/select.h>
#include <sys/types.h>

typedef uint32_t id_t;
typedef uint32_t flags_t;
typedef uint64_t event_mask_t;
typedef event_mask_t grab_type;

#define COMPOSE_DEVICE_KBD    0
#define COMPOSE_DEVICE_MOUSE  1

#define COMPOSE_DEVICE_AMOUNT 2

#define COMPOSE_WIN_FLAGS_MOVABLE            (1 << 0)
#define COMPOSE_WIN_FLAGS_RESIZABLE          (1 << 1)
#define COMPOSE_WIN_FLAGS_IN_RESIZE          (1 << 2)
#define COMPOSE_WIN_FLAGS_HAS_DELAYED_RESIZE (1 << 3)

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
	id_t       root;
	id_t       overlay;
	pid_t      pid;
	int        socket;
	time_t     ping;
} compose_client_t;

typedef struct {
	char bytes[35];
} compose_uuid;

typedef struct {
	compose_uuid  buff_id;
	int           w, h;
	size_t        data_size;
} compose_gc_t;

typedef struct {
	compose_gc_t* gc;
	void*         data;
	size_t        old_data_size;
	int           fd;
	int           buff_fd;
} compose_srv_gc_t;

typedef struct {
	compose_gc_t* gc;
	fb_t fb;
	id_t win;
	int  prev_fd;
} compose_cl_gc_t;

typedef struct window {
	id_t              id;
	compose_client_t* client;
	struct window*    parent;
	struct window*    root;
	compose_srv_gc_t  gc;
	flags_t    		  flags;
	position_t 		  pos;
	int               layer;
	sizes_t    		  sizes;
	sizes_t           delayed_resize_size;
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
	compose_window_t* overlay;
	list_t*    		  clients;
	list_t*           remove_queue;
	list_t*           windows;
	list_t*           grabs;
} compose_server_t;

typedef struct {
	int x, y;
	int w, h;
	int flags;
	event_mask_t event_mask;
} window_properties_t;


compose_client_t* compose_create_client(int sock);
compose_uuid      compose_generate_uuid(int discriminator);

compose_server_t* compose_sv_create(const char* sock);
compose_client_t* compose_sv_accept(compose_server_t* srv);
void              compose_sv_tick(compose_server_t* srv);
compose_client_t* compose_sv_get_client(compose_server_t* srv, id_t id);
void              compose_sv_disconnect(compose_server_t* srv, compose_client_t* cli);

compose_client_t*   compose_cl_connect(const char* sock);
void                compose_cl_disconnect(compose_client_t* client);
id_t                compose_cl_create_window(compose_client_t* client, id_t par, window_properties_t props);
int                 compose_cl_move(compose_client_t* cli, id_t win, int x, int y);
int                 compose_cl_layer(compose_client_t* cli, id_t win, int z);
int                 compose_cl_raise(compose_client_t* cli, id_t win);
int                 compose_cl_sunk(compose_client_t* cli, id_t win);
int                 compose_cl_resize(compose_client_t* cli, id_t win, size_t w, size_t h);
int                 compose_cl_confirm_resize(compose_client_t* cli, id_t win);
int                 compose_cl_evmask(compose_client_t* cli, id_t win, event_mask_t mask);
int                 compose_cl_grab(compose_client_t* cli, id_t win, grab_type type);
int                 compose_cl_focus(compose_client_t* cli, id_t win);
int                 compose_cl_unfocus(compose_client_t* cli, id_t win);
window_properties_t compose_cl_get_properties(compose_client_t* cli, id_t win);
compose_cl_gc_t*    compose_cl_get_gc(id_t win);
void                compose_cl_release_gc(compose_cl_gc_t* ctx);
void                compose_cl_resize_gc(compose_cl_gc_t* ctx, sizes_t new_size);
void                compose_cl_flush(compose_client_t* cli, id_t win);

void              compose_sv_move(compose_window_t* win, int x, int y, int z, uint8_t mask);
void              compose_sv_start_resize(compose_window_t* win, size_t w, size_t h);
void              compose_sv_apply_size(compose_window_t* win);
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
