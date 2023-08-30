#include <fs/tar.h>
#include <fs/vfs.h>
#include <kernel.h>
#include <mem/heap.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include "util/log.h"
#include "util/path.h"

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

static uint32_t         __k_fs_tar_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
static struct fs_node*  __k_fs_tar_finddir(fs_node_t* node, const char* path);
static struct dirent*   __k_fs_tar_readdir(fs_node_t* node, uint32_t index);

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
        size = fsize - offset;
    }

    k_free(header);

    return k_fs_vfs_read(dev, node->inode + 512 + offset, size, buffer);
}

static struct dirent* __k_fs_tar_readdir(fs_node_t* node, uint32_t index) {
	struct dirent* result = k_calloc(1, sizeof(struct dirent));

	if(index == 0) {
		strcpy(result->name, ".");
		result->ino = 0;
		return result;
	}

	if(index == 1) {
		strcpy(result->name, "..");
		result->ino = 0;
		return result;
	}

	index -= 2;

    fs_node_t* dev = (fs_node_t*) node->device;

    tar_header_t* parent_header = __k_fs_tar_read_header(dev, node->inode);

    uint32_t offs = 0;
    uint32_t i    = 0;

    char* folder = k_util_path_canonize(parent_header->filename);
    k_free(parent_header);

	uint32_t target_path_length = k_util_path_length(folder) + 1;

    while (offs < dev->size) {
        tar_header_t* header = __k_fs_tar_read_header(dev, offs);

        if(!strlen(header->filename)){
            k_free(header);
			k_free(folder);
            return 0;
        }

		uint32_t size        = __k_fs_tar_get_size(header->size);
		uint32_t path_length = k_util_path_length(header->filename);

		if(path_length == target_path_length && !strncmp(folder, header->filename, strlen(folder))) {
			if(i == index) {
				char* name = k_util_path_filename(header->filename);
				strcpy(result->name, name);
				result->ino = offs;
				k_free(name);
				k_free(header);
				return result;
			} else {
				i++;
			}
		}

		k_free(header);

		offs += ((size / 512) + 1) * 512;
		if(size % 512) {
			offs += 512;
		}
    }

    k_free(folder);

    return 0;
}

static struct fs_node* __k_fs_tar_finddir(fs_node_t* node, const char* path) {
	fs_node_t* dev = (fs_node_t*) node->device;
    tar_header_t* parent_header = __k_fs_tar_read_header(node->device, node->inode);

    char* fullpath = k_util_path_join(parent_header->filename, path); 
    k_free(parent_header);

    uint32_t offs = 0;

    while (offs < dev->size) {
        tar_header_t* header = __k_fs_tar_read_header(dev, offs);
		uint32_t size = __k_fs_tar_get_size(header->size);

        if(!strlen(header->filename)){
            k_free(header);
            return 0;
        }

        char* simplified = k_util_path_canonize(header->filename);
        if (!strcmp(simplified, fullpath)) {
            k_free(fullpath);
            k_free(simplified);
            fs_node_t* node =  __k_fs_tar_create_node(dev, header, offs);
            k_free(header);
            return node;
        }

        k_free(header);
        k_free(simplified);

		offs += ((size / 512) + 1) * 512;
		if(size % 512) {
			offs += 512;
		}
    }

    k_free(fullpath);
	k_info("--> Not found");

	return 0;
}

static fs_node_t* __k_fs_tar_mount(const char* path,
                                   const char* device) {

    fs_node_t* dev = k_fs_vfs_open(device, O_RDONLY);
    if (!dev) {
        return 0;
    }

    char* fn = k_util_path_filename(path);
	uint8_t is_root = 0;
	if(!strcmp(fn, "/")) {
		k_free(fn);
		fn = "[root]";
		is_root = 1;
	}

    fs_node_t* root = k_fs_vfs_create_node(fn);

	if(!is_root) {
		k_free(fn);	
	}

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
