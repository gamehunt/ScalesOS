#include <fs/ramdisk.h>
#include <stdio.h>
#include <string.h>
#include "fs/vfs.h"
#include "util/log.h"

static uint32_t num = 0;

static uint32_t  __k_fs_ramdisk_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer){
    uint32_t r_size = (offset + size > node->size) ? node->size : size;
    memcpy(buffer, (void*) (node->inode + offset), r_size);
    return r_size;
}

fs_node_t* k_fs_ramdisk_mount(uint32_t addr, uint32_t size){
    char path[0x1000];
    char name[0xFF];

    memset(path, 0, 0x1000);
    memset(name, 0, 0xFF);

    sprintf(name, "ram%d", num);
    sprintf(path, "/dev/%s", name);

    num++;

    k_info("Mounted ramdisk at 0x%.8x as %s", addr, path);

    fs_node_t* dev = k_fs_vfs_create_node(name);
    dev->inode     = addr;
    dev->size      = size;
    dev->fs.read   = __k_fs_ramdisk_read;

    k_fs_vfs_mount_node(path, dev);

    return dev;
}