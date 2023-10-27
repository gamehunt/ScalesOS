#ifndef __K_FS_SOCKET_H
#define __K_FS_SOCKET_H

#include "fs/vfs.h"
#include "proc/mutex.h"
#include "proc/process.h"
#include <sys/socket.h>

#define SOCKET_MAX_BUFFER_SIZE  256 * 256
#define SOCKET_INIT_BUFFER_SIZE 4096

#define SOCK_SEL_QUEUE_R 0
#define SOCK_SEL_QUEUE_W 1
#define SOCK_SEL_QUEUE_E 2

typedef struct {
	struct sockaddr* addr;
	socklen_t        addr_len;
	void*            bind;
	void*            connect;
	int              domain;
	int              type;
	int              listening;
	list_t*          backlog;
	void*            buffer;
	uint32_t         buffer_size;
	uint32_t         available_data;
	list_t*          cnn_blocked_processes;
	list_t*          blocked_processes;
	mutex_t          lock;
	list_t*          sel_queues[3];
} socket_t;

fs_node_t* k_fs_socket_create(int domain, int type, int protocol);
int        k_fs_socket_bind(socket_t* socket, struct sockaddr* addr, socklen_t l);
int        k_fs_socket_connect(socket_t* socket, struct sockaddr* addr, socklen_t l);
int        k_fs_socket_listen(socket_t* socket);
fs_node_t* k_fs_socket_accept(socket_t* socket);
#endif
