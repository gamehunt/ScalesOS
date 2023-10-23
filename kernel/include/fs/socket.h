#ifndef __K_FS_SOCKET_H
#define __K_FS_SOCKET_H

#include "fs/vfs.h"

fs_node_t* k_fs_socket_create(int domain, int type, int protocol);

#endif
