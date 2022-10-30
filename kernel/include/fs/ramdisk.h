#ifndef __K_FS_TAR_H
#define __K_FS_TAR_H

#include "fs/vfs.h"
#include "kernel.h"

K_STATUS   k_fs_ramdisk_init();
fs_node_t* k_fs_ramdisk_load(uint32_t addr, const char* mount_path);

#endif