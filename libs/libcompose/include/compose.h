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
	fb_t       framebuffer;
	int        socket;
	flags_t    flags;
	position_t pos;
	sizes_t    sizes;
} compose_client_t;

typedef struct {
	int  socket;
	fb_t framebuffer;
	int  devices[COMPOSE_DEVICE_AMOUNT];
	list_t* clients;
} compose_server_t;

compose_server_t* compose_sv_create(const char* sock);
compose_client_t* compose_sv_accept(compose_server_t* srv);
compose_client_t* compose_create_client(int sock);
void              compose_sv_close(compose_server_t* srv, compose_client_t* cli);

compose_client_t* compose_connect(const char* sock);
void              compose_cl_disconnect(compose_client_t* client);
void              compose_sv_tick(compose_server_t* srv);

int               compose_cl_move(compose_client_t* cli, int x, int y);
int               compose_cl_layer(compose_client_t* cli, int z);
int               compose_cl_resize(compose_client_t* cli, size_t w, size_t h);

void              compose_sv_move(compose_client_t* cli, int x, int y, int z);
void              compose_sv_resize(compose_client_t* cli, size_t w, size_t h);

#endif
