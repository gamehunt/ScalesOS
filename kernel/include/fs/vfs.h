#ifndef __FS_VFS_H
#define __FS_VFS_H

#include <stdint.h>
#include "kernel.h"

#define VFS_FILE       (1 << 0)
#define VFS_DIR        (1 << 1)
#define VFS_MOUNTPOINT (1 << 2)

struct fs_node;
typedef struct fs_node fs_node_t;

typedef struct fs{
    uint32_t        (*write)   (fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
    uint32_t        (*read)    (fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
    void            (*open)    (fs_node_t* node);
    void            (*close)   (fs_node_t* node);
    struct fs_node* (*finddir) (fs_node_t* node, const char* name);
}fs_t;

struct fs_node{
    char     name[256];
    uint32_t flags;
    fs_t*    fs;
    uint64_t inode;
    uint32_t size;
};

typedef struct vfs_entry{
    char name[256];
    fs_node_t* node;
}vfs_entry_t;

K_STATUS     k_fs_vfs_init         ();
fs_node_t*   k_fs_vfs_create_node  (const char* name);
vfs_entry_t* k_fs_vfs_create_entry (const char* name);
vfs_entry_t* k_fs_vfs_map_path     (const char* path);
uint32_t     k_fs_vfs_read         (fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
uint32_t     k_fs_vfs_write        (fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
fs_node_t*   k_fs_vfs_finddir      (fs_node_t* node, const char* path);
K_STATUS     k_fs_vfs_mount        (const char* path, fs_node_t* root);
fs_node_t*   k_fs_vfs_open         (const char* path);
void         k_fs_vfs_close        (fs_node_t* node);
void         k_d_fs_vfs_print      ();
void         k_fs_vfs_register_fs  (const char* alias, fs_t* fs);
fs_t*        k_fs_vfs_get_fs       (const char* alias);
#endif