#include "fs/socket.h"
#include "errno.h"
#include "fs/vfs.h"
#include "mem/heap.h"
#include "proc/mutex.h"
#include "proc/process.h"
#include "sys/socket.h"
#include "sys/un.h"
#include "util/log.h"
#include "util/types/list.h"

#include <string.h>
#include <stdio.h>

static uint32_t __k_fs_socket_read(fs_node_t* node, uint32_t offset UNUSED, uint32_t size, uint8_t* buffer) {
	socket_t* sock = node->device;
	if(!sock) {
		return 0;
	}

	uint32_t data_left = size;
	while(data_left) {
		while(!sock->available_data) { // TODO nonblocking sockets
			k_proc_process_sleep_on_queue(k_proc_process_current(), sock->blocked_processes);
		}
		uint32_t to_read = 0;
		if(size < sock->available_data) {
			to_read = size;
		} else {
			to_read = sock->available_data;
		}
		mutex_lock(&sock->lock);
		memcpy(buffer, sock->buffer, to_read);
		sock->available_data -= to_read;
		if(sock->available_data) {
			memmove(sock->buffer, sock->buffer + to_read, sock->available_data);
		} 
		mutex_unlock(&sock->lock);
		data_left -= to_read;
	}

	return size;
}

static uint32_t __k_fs_socket_write(fs_node_t* node, uint32_t offset UNUSED, uint32_t size, uint8_t* buffer) {
	socket_t* sock = node->device;

	if(!sock) {
		return 0;
	}

	if(sock->domain != AF_LOCAL) {
		return -EINVAL;
	}

	if(!sock->connect) {
		return -ENOTCONN;
	}

	socket_t* client = sock->connect;

	mutex_lock(&client->lock);

	if(client->available_data >= client->buffer_size) {
		if(client->available_data >= SOCKET_MAX_BUFFER_SIZE) {
			return -ENOBUFS;
		}

		client->buffer_size += SOCKET_INIT_BUFFER_SIZE;
		client->buffer = k_realloc(client->buffer, client->buffer_size);
	}

	memcpy(client->buffer + client->available_data, buffer, size);
	client->available_data += size;

	mutex_unlock(&client->lock);

	k_proc_process_wakeup_queue(client->blocked_processes);

	return size;
}

fs_node_t* k_fs_socket_create(int domain, int type, int protocol UNUSED) {
	if(domain != AF_LOCAL) {
		return NULL;
	}

	fs_node_t* node = k_fs_vfs_create_node("[socket]");

	socket_t* sock    = k_calloc(1, sizeof(socket_t));
	sock->domain      = domain;
	sock->type        = type;
	sock->buffer_size = SOCKET_INIT_BUFFER_SIZE;
	sock->buffer      = k_malloc(sock->buffer_size);
	sock->backlog     = list_create();
	sock->blocked_processes = list_create();
	sock->cnn_blocked_processes = list_create();

	mutex_init(&sock->lock);

	node->device   = sock;
	node->fs.write = &__k_fs_socket_write;
	node->fs.read  = &__k_fs_socket_read;
	node->mode     = O_RDWR;

	return node;
}

int k_fs_socket_bind(socket_t* socket, struct sockaddr* addr, socklen_t l) {
	if(socket->bind) {
		return -EINVAL;
	}

	if(socket->domain == AF_LOCAL) {
		socket->addr = k_malloc(l);
		memcpy(socket->addr, addr, l);	
		socket->addr_len = l;

		fs_node_t* sock = k_fs_vfs_create_node("[local-socket]");	
		sock->device    = socket;

		struct sockaddr_un* un = (struct sockaddr_un*) addr;

		k_fs_vfs_mount_node(un->sun_path, sock);

		socket->bind    = sock;

		return 0;
	}

	return -EINVAL;
}

int k_fs_socket_connect(socket_t* socket, struct sockaddr* addr, socklen_t l) {
	if(socket->connect) {
		return -EISCONN;
	}

	if(socket->domain == AF_LOCAL) {
		struct sockaddr_un* un = (struct sockaddr_un*) addr;
		fs_node_t* remote_sock = k_fs_vfs_open(un->sun_path, O_RDWR);

		if(!remote_sock) {
			return -ENOENT;
		}

		socket_t* server = remote_sock->device;
		if(!server->listening) {
			return -ECONNREFUSED;
		}

		socket->addr_len = l;
		if(socket->addr) {
			k_free(socket->addr);
		}
		socket->addr = k_malloc(l);
		memcpy(socket->addr, addr, l);

		list_push_back(server->backlog, socket);

		k_proc_process_wakeup_queue(server->blocked_processes);

		while(!socket->connect) {
			k_proc_process_sleep_on_queue(k_proc_process_current(), server->cnn_blocked_processes);
		}

		if(socket->connect == 0xFFFFFFFF) {
			socket->connect = 0;
			return -EINVAL;
		}

		return 0;
	}

	return -EINVAL;
}

int k_fs_socket_listen(socket_t* socket) {
	socket->listening = 1;

	return 0;
}

fs_node_t* k_fs_socket_accept(socket_t* socket) {
	if(socket->domain == AF_LOCAL) {
		while(!socket->backlog->size) {
			k_proc_process_sleep_on_queue(k_proc_process_current(), socket->blocked_processes);
		}

		socket_t* client      = list_pop_front(socket->backlog);

		fs_node_t* node       = k_fs_socket_create(socket->domain, socket->type, 0);		
		socket_t*  new_socket = node->device; 
		client->connect       = new_socket;

		new_socket->connect   = client;
		new_socket->addr_len  = client->addr_len;
		new_socket->addr = k_malloc(new_socket->addr_len);
		memcpy(new_socket->addr, client->addr, client->addr_len);

		k_proc_process_wakeup_queue(socket->cnn_blocked_processes);

		return node;
	}

	return NULL;
}
