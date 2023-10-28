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

static int __k_fs_socket_event2index(uint8_t event) {
	switch(event) {
		case VFS_EVENT_READ:
			return SOCK_SEL_QUEUE_R;
		case VFS_EVENT_WRITE:
			return SOCK_SEL_QUEUE_W;
		case VFS_EVENT_EXCEPT:
			return SOCK_SEL_QUEUE_E;
		default:
			return -EINVAL;
	}
}

static uint8_t __k_fs_socket_check(fs_node_t* node, uint8_t event) {
	socket_t* socket = node->device;
	if(!socket) {
		return 0;
	}

	switch(event) {
		case VFS_EVENT_READ:
			return socket->available_data > 0;
		case VFS_EVENT_WRITE:
			return socket->available_data < SOCKET_MAX_BUFFER_SIZE;
		default:
			return 0;
	}
}

static int __k_fs_socket_wait(fs_node_t* node, uint8_t event, process_t* prc) {
	socket_t* socket = node->device;
	if(!socket) {
		return -ENOENT;
	}

	int idx = __k_fs_socket_event2index(event);
	if(idx < 0) {
		return idx;
	}

	if(!list_contains(socket->sel_queues[idx], prc->block_node)) {
		k_proc_process_own_block(prc, socket->sel_queues[idx]);
	}

	return 0;
}
static void __k_fs_socket_notify(fs_node_t* node, uint8_t event) {
	socket_t* p = node->device;
	int idx = __k_fs_socket_event2index(event);
	if(idx < 0) {
		return;
	}
	k_proc_process_wakeup_queue_select(p->sel_queues[idx], node, event);
}

static uint32_t __k_fs_socket_read(fs_node_t* node, uint32_t offset UNUSED, uint32_t size, uint8_t* buffer) {
	socket_t* sock = node->device;
	if(!sock) {
		return 0;
	}

	uint32_t data_left = size;
	while(data_left) {
		uint8_t b = 0;
		while(!sock->available_data) {
			if(node->mode & O_NOBLOCK) {
				b = 1;
				break;
			}
			k_proc_process_sleep_on_queue(k_proc_process_current(), sock->blocked_processes);
		}
		if(b) {
			break;
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

	__k_fs_socket_notify(node, VFS_EVENT_WRITE);

	return size - data_left;
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
			__k_fs_socket_notify(node, VFS_EVENT_EXCEPT);
			return -ENOBUFS;
		}

		client->buffer_size += SOCKET_INIT_BUFFER_SIZE;
		client->buffer = k_realloc(client->buffer, client->buffer_size);
	}

	memcpy(client->buffer + client->available_data, buffer, size);
	client->available_data += size;

	mutex_unlock(&client->lock);

	k_proc_process_wakeup_queue(client->blocked_processes);

	__k_fs_socket_notify(node, VFS_EVENT_READ);

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

	for(int i = 0; i < 3; i++) {
		sock->sel_queues[i] = list_create();
	}

	mutex_init(&sock->lock);

	node->device   = sock;
	node->fs.write = &__k_fs_socket_write;
	node->fs.read  = &__k_fs_socket_read;
	node->fs.check = &__k_fs_socket_check;
	node->fs.wait  = &__k_fs_socket_wait;
	node->mode     = O_RDWR;
	if(type & SOCK_NONBLOCK) {
		node->mode |= O_NOBLOCK;
	}

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
			if(socket->type & SOCK_NONBLOCK) {
				return -EAGAIN;
			}
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
			if(socket->type & SOCK_NONBLOCK) {
				return (fs_node_t*) -EAGAIN;
			}
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

	return (fs_node_t*) -ENOTSUP;
}
