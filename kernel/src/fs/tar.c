#include <fs/tar.h>
#include <fs/vfs.h>
#include <kernel.h>
#include <mem/heap.h>
#include <stdio.h>
#include <string.h>
#include "util/log.h"
#include "util/path.h"

static uint32_t __k_fs_tar_get_size(char sz[12]) {
    unsigned int size = 0;
    unsigned int j;
    unsigned int count = 1;

    for (j = 11; j > 0; j--, count *= 8) size += ((sz[j - 1] - '0') * count);

    return size;
}

static tar_header_t* __k_fs_tar_read_header(fs_node_t* dev, uint32_t offset) {
    tar_header_t* header = k_malloc(sizeof(tar_header_t));
    k_fs_vfs_read(dev, offset, sizeof(tar_header_t), (uint8_t*)header);
    return header;
}

static uint32_t __k_fs_tar_read(fs_node_t* node, uint32_t offset, uint32_t size,
                                uint8_t* buffer);
static struct fs_node* __k_fs_tar_finddir(fs_node_t* node, const char* path);
static struct dirent* __k_fs_tar_readdir(fs_node_t* node, uint32_t index);

static fs_node_t* __k_fs_tar_create_node(fs_node_t* device,
                                         tar_header_t* header,
                                         uint32_t offset) {
    char* buffer = k_util_path_filename(header->filename);
    fs_node_t* node = k_fs_vfs_create_node(buffer);
    k_free(buffer);
    node->device = device;
    node->inode  = offset;
    node->size = __k_fs_tar_get_size(header->size);
    node->fs.read    = __k_fs_tar_read;
    node->fs.finddir = __k_fs_tar_finddir;
    node->fs.readdir = __k_fs_tar_readdir;
    return node;
}

static uint32_t __k_fs_tar_read(fs_node_t* node, uint32_t offset, uint32_t size,
                                uint8_t* buffer) {
    fs_node_t* dev = node->device;

    tar_header_t* header = __k_fs_tar_read_header(dev, node->inode);
    uint32_t fsize = __k_fs_tar_get_size(header->size);

    if (offset > fsize) {
        k_free(header);
        return 0;
    }

    if (fsize < offset + size) {
        size = fsize;
    }

    k_free(header);

    return k_fs_vfs_read(dev, node->inode + offset + 512, size, buffer);
}

static struct dirent* __k_fs_tar_readdir(fs_node_t* node, uint32_t index) {
    fs_node_t* dev = (fs_node_t*) node->device;

    tar_header_t* parent_header =
        __k_fs_tar_read_header(dev, node->inode);

    uint32_t offs = 0;
    uint32_t i    = 0;

    char* folder = k_util_path_canonize(parent_header->filename);
    k_free(parent_header);

    while (offs < dev->size) {
        tar_header_t* header = __k_fs_tar_read_header(dev, offs);

        if(!strlen(header->filename)){
            k_free(header);
            return 0;
        }

        char* simplified = k_util_path_canonize(header->filename);
        if (!strncmp(simplified, folder, strlen(folder)) && strcmp(simplified, folder)) {
            if(i == index){
                k_free(folder);
                k_free(header);
                k_free(simplified);
                
                struct dirent* dir = k_malloc(sizeof(struct dirent));

                char* name = k_util_path_filename(simplified);
                strcpy(dir->name, name);
                k_free(name);

                dir->ino = offs;

                return dir;
            }else{
                i++;
            }
        }

        k_free(header);
        k_free(simplified);

        offs += 512;
        if (offs % 512) {
            offs += (512 - offs % 512);
        }
    }

    k_free(folder);

    return 0;
}

static struct fs_node* __k_fs_tar_finddir(fs_node_t* node, const char* path) {
    fs_node_t* dev = (fs_node_t*)node->device;
    tar_header_t* parent_header =
        __k_fs_tar_read_header(node->device, node->inode);

    char* fullpath = k_util_path_join(parent_header->filename, path); 
    k_free(parent_header);

    uint32_t offs = 0;

    while (offs < dev->size) {
        tar_header_t* header = __k_fs_tar_read_header(dev, offs);

        if(!strlen(header->filename)){
            k_free(header);
            return 0;
        }

        char* simplified = k_util_path_canonize(header->filename);
        if (!strcmp(simplified, fullpath)) {
            k_free(fullpath);
            k_free(header);
            k_free(simplified);
            return __k_fs_tar_create_node(dev, header, offs);
        }

        k_free(header);
        k_free(simplified);

        offs += 512;
        if (offs % 512) {
            offs += (512 - offs % 512);
        }
    }

    k_free(fullpath);

    return 0;
}

static fs_node_t* __k_fs_tar_mount(const char* path UNUSED,
                                   const char* device) {
    fs_node_t* dev = k_fs_vfs_open(device);
    if (!dev) {
        return 0;
    }

    fs_node_t* root = k_malloc(sizeof(fs_node_t));
    memset(root, 0, sizeof(fs_node_t));
    tar_header_t* root_header = __k_fs_tar_read_header(dev, 0);
    char* fn = k_util_path_filename(root_header->filename);
    strcpy(root->name, fn);
    k_free(fn);
    k_free(root_header);

    root->device = dev;
    root->fs.finddir = __k_fs_tar_finddir;
    root->fs.readdir = __k_fs_tar_readdir;
    root->flags = VFS_DIR;

    return root;
}

K_STATUS k_fs_tar_init() {
    k_fs_vfs_register_fs("tar", __k_fs_tar_mount);
    return K_STATUS_OK;
}