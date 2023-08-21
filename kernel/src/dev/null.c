#include "dev/null.h"
#include "fs/vfs.h"
#include "kernel.h"
#include <string.h>


static uint32_t  __k_dev_null_null_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer){
    return 0;
}

static uint32_t  __k_dev_null_null_write(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer){
    return 0;
}

static uint32_t  __k_dev_null_zero_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer){
	memset(buffer + offset, 0, size);
    return size;
}

static uint32_t  __k_dev_null_zero_write(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer){
    return size;
}

static fs_node_t* __k_dev_null_create_null_device() {
	fs_node_t* node = k_fs_vfs_create_node("null");
	node->fs.read  = __k_dev_null_null_read;
	node->fs.write = __k_dev_null_null_write;
	return node;
}

static fs_node_t* __k_dev_null_create_zero_device() {
	fs_node_t* node = k_fs_vfs_create_node("null");
	node->fs.read  = __k_dev_null_zero_read;
	node->fs.write = __k_dev_null_zero_write;
	return node;
}

K_STATUS k_dev_null_init() {
	k_fs_vfs_mount_node("/dev/null", __k_dev_null_create_null_device());
	k_fs_vfs_mount_node("/dev/zero", __k_dev_null_create_zero_device());

	return K_STATUS_OK; 
}
