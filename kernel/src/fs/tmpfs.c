#include "fs/tmpfs.h"
#include "fs/vfs.h"
#include "kernel.h"
#include "mem/heap.h"
#include "util/path.h"
#include "util/types/list.h"
#include <string.h>

typedef struct tmpfs_node {
	char   name[256];
	struct tmpfs_node* parent;
	list_t* children;
} tmpfs_node_t;

static tmpfs_node_t* __k_fs_tmpfs_create_node(char* name, tmpfs_node_t* parent) {
	tmpfs_node_t* node = k_calloc(1, sizeof(tmpfs_node_t));
	strcpy(node->name, name);
	node->children = list_create();
	if(parent) {
		node->parent = parent;
		list_push_back(parent->children, node);
	}
	return node;
} 

static fs_node_t* __k_fs_tmpfs_create_root(char* name) {
	tmpfs_node_t* tmpfs_root = __k_fs_tmpfs_create_node(name, 0);
	fs_node_t* fs_root = k_fs_vfs_create_node(name);
	fs_root->device = tmpfs_root;
	return fs_root;
}

static fs_node_t* __k_fs_tmpfs_mount(const char* mountpoint, const char* device UNUSED) {
	return __k_fs_tmpfs_create_root(k_util_path_filename(mountpoint));
}

K_STATUS k_fs_tmpfs_init() {
	k_fs_vfs_register_fs("tmpfs", __k_fs_tmpfs_mount);
	return K_STATUS_OK;
}
