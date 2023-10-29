#ifndef __LIB_COMPOSE_H
#define __LIB_COMPOSE_H

#include "fb.h"
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
	int        socket;
	flags_t    flags;
	position_t pos;
	sizes_t    sizes;
} compose_client_t;

typedef struct {
	int  socket;
	fb_t framebuffer;
	int  devices[COMPOSE_DEVICE_AMOUNT];
	compose_client_t** clients;
	size_t             clients_amount;
} compose_server_t;

compose_server_t* compose_server_create(const char* sock);
compose_client_t* compose_server_accept(compose_server_t* srv);
compose_client_t* compose_server_create_client(int sock);
void              compose_server_close(compose_server_t* srv, compose_client_t* cli);

compose_client_t* compose_connect(const char* sock);
void              compose_server_tick(compose_server_t* srv);
void              compose_server_redraw(compose_client_t* client); 

int               compose_client_move(compose_client_t* cli, int x, int y);
int               compose_client_layer(compose_client_t* cli, int z);
int               compose_client_resize(compose_client_t* cli, size_t w, size_t h);

#endif
