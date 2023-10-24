#include "fs/socket.h"
#include "fs/vfs.h"

fs_node_t* k_fs_socket_create(int domain, int type, int protocol) {
	fs_node_t* node = k_fs_vfs_create_node("[socket]");
	return node;
}

