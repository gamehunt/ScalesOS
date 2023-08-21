#include "dev/tty.h"
#include "fs/vfs.h"
#include "kernel.h"

static uint32_t __k_dev_tty_readlink(fs_node_t* node, uint8_t* buffer, uint32_t size) {

}

static fs_node_t* __k_dev_tty_create_device() {
	fs_node_t* node = k_fs_vfs_create_node("tty");
	node->flags = VFS_FILE | VFS_SYMLINK;
	node->fs.readlink = __k_dev_tty_readlink;	
	return node;
}

static fs_node_t* __k_dev_tty_create_pty_root() {
	fs_node_t* node = k_fs_vfs_create_node("pts");
	node->flags = VFS_DIR;
	return node;
}

static fs_node_t* __k_dev_tty_create_pty(uint32_t id) {

}

K_STATUS k_dev_tty_init() {
	k_fs_vfs_mount_node("/dev/tty", __k_dev_tty_create_device());
	k_fs_vfs_mount_node("/dev/pts", __k_dev_tty_create_pty_root());	
	return K_STATUS_OK;
}
