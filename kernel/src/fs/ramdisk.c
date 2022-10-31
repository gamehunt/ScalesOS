#include <fs/ramdisk.h>
#include <stdio.h>
#include <string.h>
#include "fs/vfs.h"
#include "kernel.h"
#include "mem/heap.h"
#include "shared.h"
#include "util/log.h"

struct tar_header
{
    char filename[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag[1];
};

typedef struct tar_header tar_header_t;

static tar_header_t** headers;
static fs_t*          fs;
static fs_node_t*     fs_node;

static uint32_t __k_fs_ramdisk_get_size(char sz[12]){
    unsigned int size = 0;
    unsigned int j;
    unsigned int count = 1;
 
    for (j = 11; j > 0; j--, count *= 8)
        size += ((sz[j - 1] - '0') * count);
 
    return size;
}

static fs_node_t* __k_fs_ramdisk_create_root_node(){
    fs_node_t* node = k_fs_vfs_create_node("[ramdisk_root]");
    node->flags = VFS_DIR;
    node->fs = fs;
    return node;
}

static fs_node_t* __k_fs_ramdisk_create_node(tar_header_t* header){
    uint32_t last_idx = strlen(header->filename) - 1;
    while(header->filename[last_idx] != '/'){
        last_idx--;
    }
    fs_node_t* node = k_fs_vfs_create_node(&header->filename[last_idx]);
    node->flags = VFS_DIR;
    node->fs    = fs;   
    node->size  = __k_fs_ramdisk_get_size(header->size);
    node->inode = (uint32_t) header + 512; 
    return node;
}

static uint32_t __k_fs_ramdisk_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer){
    memcpy(buffer, (void*) ((uint32_t)node->inode + offset), size);
    return size;
}

static struct fs_node* __k_fs_ramdisk_finddir(fs_node_t* node UNUSED, const char* path){
    uint32_t i = 0;
    char fullpath[0x1000];
    if(node == fs_node){ //TODO proper path parsing
        sprintf(fullpath, "ramdisk/%s", path);
    }else{
        sprintf(fullpath, "ramdisk/%s/%s", node->name, path);
    }
    while(headers[i]){
        if(!strcmp(headers[i]->filename, fullpath)){
            return __k_fs_ramdisk_create_node(headers[i]);
        }
        i++;
    }
    return 0;
}

static void __k_fs_ramdisk_close(fs_node_t* node){
    k_free(node);
}

static tar_header_t** __k_fs_ramdisk_parse(uint32_t addr){
    uint32_t  i;
    tar_header_t** headers;

    for (i = 0; ; i++)
    {
        struct tar_header *header = (struct tar_header *)addr;

        uint32_t count = i;
        EXTEND(headers, count, sizeof(tar_header_t*));

        if (header->filename[0] == '\0'){
            headers[i] = 0;
            break;
        }else{
            headers[i] = header;
        }

        k_debug("Ramdisk file: %s", headers[i]->filename);
 
        uint32_t size = __k_fs_ramdisk_get_size(header->size);
        
        addr += ((size / 512) + 1) * 512;
        if (size % 512)
            addr += 512;
    }
 
    return headers;
}


K_STATUS k_fs_ramdisk_init(){
    fs = k_malloc(sizeof(fs_t));
    fs->close   = __k_fs_ramdisk_close;
    fs->open    = 0;
    fs->finddir = __k_fs_ramdisk_finddir;
    fs->read    = __k_fs_ramdisk_read;
    fs->write   = 0;
    k_fs_vfs_register_fs("ramdisk", fs);
    return K_STATUS_OK;
}

fs_node_t* k_fs_ramdisk_load(uint32_t addr, const char* mount_path){
    k_info("Loading ramdisk (will be mounted to %s)...", mount_path);
    headers = __k_fs_ramdisk_parse(addr);
    fs_node = __k_fs_ramdisk_create_root_node();
    k_fs_vfs_mount(mount_path, fs_node);
    return fs_node;
}