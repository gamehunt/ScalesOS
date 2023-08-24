#ifndef __FS_VFS_H
#define __FS_VFS_H

#include <stdint.h>
#include <kernel.h>
#include <shared.h>

#define VFS_FILE       (1 << 0)
#define VFS_DIR        (1 << 1)
#define VFS_MOUNTPOINT (1 << 2)
#define VFS_SYMLINK    (1 << 3)

struct fs_node;
typedef struct fs_node fs_node_t;

typedef uint32_t       (*fs_write_t)    (fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
typedef uint32_t       (*fs_read_t)     (fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
typedef void           (*fs_open_t)     (fs_node_t* node, uint8_t mode);
typedef void           (*fs_close_t)    (fs_node_t* node);
typedef fs_node_t*     (*fs_finddir_t)  (fs_node_t* node, const char* name);
typedef uint32_t       (*fs_readlink_t) (fs_node_t* node, uint8_t* buf, uint32_t size);
typedef struct dirent* (*fs_readdir_t)  (fs_node_t* node, uint32_t index);

typedef struct fs_ops{
    fs_write_t    write;
    fs_read_t     read;
    fs_open_t     open;
    fs_close_t    close;
    fs_finddir_t  finddir;
    fs_readdir_t  readdir;
	fs_readlink_t readlink;
}fs_ops_t;

struct fs_node{
    char     name[256];
    void*    device;
	uint8_t  mode;
    uint32_t flags;
    uint64_t inode;
    uint32_t size;
    fs_ops_t fs;
};

typedef struct vfs_entry{
    char name[256];
    fs_node_t* node;
}vfs_entry_t;

typedef fs_node_t* (*mount_callback) (const char* mountpoint, const char* device);

K_STATUS         k_fs_vfs_init         ();
fs_node_t*       k_fs_vfs_create_node  (const char* name);
vfs_entry_t*     k_fs_vfs_create_entry (const char* name);
vfs_entry_t*     k_fs_vfs_map_path     (const char* path);
uint32_t         k_fs_vfs_read         (fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
uint32_t         k_fs_vfs_write        (fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
fs_node_t*       k_fs_vfs_finddir      (fs_node_t* node, const char* path);
struct dirent*   k_fs_vfs_readdir      (fs_node_t* node, uint32_t index);
uint32_t         k_fs_vfs_readlink     (fs_node_t* node, uint8_t* buf, uint32_t size);
K_STATUS         k_fs_vfs_mount_node   (const char* path, fs_node_t* root);
K_STATUS         k_fs_vfs_mount        (const char* path, const char* device, const char* type);
fs_node_t*       k_fs_vfs_open         (const char* path, uint8_t mode);
void             k_fs_vfs_close        (fs_node_t* node);
void             k_d_fs_vfs_print      ();
void             k_fs_vfs_register_fs  (const char* alias, mount_callback m);
#endif
