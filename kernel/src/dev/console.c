#include <dev/console.h>
#include "fs/vfs.h"
#include "kernel.h"

console_output_source_t src = 0;

uint32_t  __k_dev_console_write(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer){
    if(src){
        src((char*) buffer, size);
    }

    return size;
}

fs_node_t* __k_dev_console_create_device(){
    fs_node_t* node = k_fs_vfs_create_node("console");
    node->fs.write = __k_dev_console_write;
    return node;
}

K_STATUS k_dev_console_init(){
    fs_node_t* dev = __k_dev_console_create_device();
    k_fs_vfs_mount_node("/dev/console", dev);
    return K_STATUS_OK;
}

void k_dev_console_set_source(console_output_source_t s){
    src = s;
}