#include "fs/tmpfs.h"
#include "dirent.h"
#include "fs/vfs.h"
#include "kernel.h"
#include "mem/heap.h"
#include "mem/paging.h"
#include "util/path.h"
#include "util/types/list.h"
#include <string.h>

#define TMPFS_BLOCK_SIZE 4096

#define TMPFS_TYPE_FILE (1 << 0)
#define TMPFS_TYPE_DIR  (1 << 1)

typedef struct tmpfs_node {
	char       name[256];
	uint8_t    type;
	uint32_t   size;
	uint32_t   blocks_amount;
	uint32_t   block_array_size;
	uint32_t** blocks;
	uint32_t   mappings;
	uint32_t   last_map;
	struct tmpfs_node* parent;
	list_t*            children;
} tmpfs_node_t;

static uint32_t __k_fs_tmpfs_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
	if(!size) {
		return 0;
	}

	if(node->size < size) {
		size = node->size;
	}

	if(size + offset > node->size) {
		size = node->size - offset;
		if(!size) {
			return 0;
		}
	}

	tmpfs_node_t* tmp_node = node->device;

	uint32_t full_blocks  = size / TMPFS_BLOCK_SIZE;
	uint32_t part_size  = size % TMPFS_BLOCK_SIZE;
	uint32_t block_offset = offset / TMPFS_BLOCK_SIZE;
	uint32_t part_offset  = offset % TMPFS_BLOCK_SIZE;

	uint32_t prev = k_mem_paging_get_pd(1);
	k_mem_paging_set_pd(tmp_node->mappings, 1, 0);

	for(uint32_t i = 0; i < full_blocks; i++) {
		if(!tmp_node->blocks[block_offset + i]) {
			return i * TMPFS_BLOCK_SIZE;
		}
		memcpy(&buffer[i * TMPFS_BLOCK_SIZE], &tmp_node->blocks[block_offset + i][part_offset], TMPFS_BLOCK_SIZE);
		part_offset = 0;
	}

	if(!tmp_node->blocks[block_offset + full_blocks]) {
		return full_blocks * TMPFS_BLOCK_SIZE;
	}

	memcpy(&buffer[full_blocks * TMPFS_BLOCK_SIZE], &tmp_node->blocks[block_offset + full_blocks][part_offset], part_size);

	k_mem_paging_set_pd(prev, 1, 0);

	return size;
}

static uint32_t __k_fs_tmpfs_allocate_blocks(tmpfs_node_t* node, uint32_t blocks) {
	if(!node->blocks_amount) {
		node->blocks_amount = 1; 
		node->block_array_size = 1;
		node->blocks = malloc(sizeof(uint32_t*));
	}

	if(node->blocks_amount + blocks <= node->block_array_size) {
		node->blocks            = k_realloc(node->blocks, sizeof(uint32_t*) * node->block_array_size * 2);
		node->block_array_size *= 2;
	}

	uint32_t allocated = 0;

	for(uint32_t i = 0; i < blocks; i++) {
		if(node->last_map + 0x1000 == VIRTUAL_BASE) {
			break;
		}
		k_mem_paging_map_region(node->last_map, 0, 1, 3, 0);
		node->blocks[node->blocks_amount + i] = (uint32_t*) (node->last_map + TMPFS_BLOCK_SIZE); 
		node->last_map += TMPFS_BLOCK_SIZE;
		allocated++;
	}

	node->blocks_amount += allocated;
	return allocated;
}

static uint32_t __k_fs_tmpfs_write(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
	tmpfs_node_t* tmp_node = node->device;

	uint32_t prev = k_mem_paging_get_pd(1);
	k_mem_paging_set_pd(tmp_node->mappings, 1, 0);

	uint32_t block_offset = offset / TMPFS_BLOCK_SIZE;
	uint32_t part_offset = offset % TMPFS_BLOCK_SIZE;

	uint32_t full_blocks_required = size / TMPFS_BLOCK_SIZE;
	uint32_t part_block_required = size % TMPFS_BLOCK_SIZE;
	if(part_offset <= part_block_required) {
		part_block_required -= part_offset;
	} else {
		part_block_required = 0;
	}

	uint32_t required_blocks = full_blocks_required - block_offset + (part_offset > 0) + (part_block_required > 0);
	
	if(tmp_node->blocks_amount < required_blocks) {
		__k_fs_tmpfs_allocate_blocks(tmp_node, required_blocks - tmp_node->blocks_amount);
	}

	uint32_t part_size = 0;
	if(part_offset) {
		part_size = TMPFS_BLOCK_SIZE - part_offset;
		if(part_size > size) {
			part_size = size;
		}
		memcpy(buffer, &tmp_node->blocks[block_offset][part_offset], part_size);
	}

	for(uint32_t i = 0; i < full_blocks_required; i++) {
		memcpy(&buffer[part_size + i * TMPFS_BLOCK_SIZE], tmp_node->blocks[block_offset + (part_offset > 0) + i], TMPFS_BLOCK_SIZE);
	}

	if(part_block_required) {
		memcpy(&buffer[part_size + full_blocks_required * TMPFS_BLOCK_SIZE], tmp_node->blocks[block_offset + (part_offset > 0) + full_blocks_required], part_block_required);
	}

	k_mem_paging_set_pd(prev, 1, 0);
	return size;
}

static struct dirent* __k_fs_tmpfs_readdir(fs_node_t* root, uint32_t index) {
	struct dirent* dir = k_malloc(sizeof(struct dirent));

	if(index == 0) {
		strcpy(dir->name, ".");
		dir->ino = 0;
		return dir;
	}

	if(index == 1) {
		strcpy(dir->name, "..");
		dir->ino = 1;
		return dir;
	}

	index -= 2;

	tmpfs_node_t* dev = root->device;

	if(dev->children->size < index) {
		tmpfs_node_t* child = dev->children->data[index];
		strcpy(dir->name, child->name);
		dir->ino = index;
		return dir;
	}

	k_free(dir);
	return NULL;
}


static fs_node_t* __k_fs_tmpfs_finddir(fs_node_t* root, const char* path);
static fs_node_t* __k_fs_tmpfs_create(fs_node_t* root, const char* path, uint8_t mode UNUSED);
static fs_node_t* __k_fs_tmpfs_mkdir(fs_node_t* root, const char* path, uint8_t mode UNUSED);

static tmpfs_node_t* __k_fs_tmpfs_create_node(char* name, tmpfs_node_t* parent) {
	tmpfs_node_t* node = k_calloc(1, sizeof(tmpfs_node_t));
	strcpy(node->name, name);
	node->mappings = k_mem_paging_clone_root();
	node->last_map = 0x10000000;
	node->children = list_create();
	if(parent) {
		node->parent = parent;
		list_push_back(parent->children, node);
	}
	return node;
} 

static fs_node_t* __k_fs_tmpfs_to_fs_node(tmpfs_node_t* node) {
	fs_node_t* fsnode = k_fs_vfs_create_node(node->name);
	fsnode->device = node;
	fsnode->fs.readdir = &__k_fs_tmpfs_readdir;
	fsnode->fs.finddir = &__k_fs_tmpfs_finddir;
	fsnode->fs.read    = &__k_fs_tmpfs_read;
	fsnode->fs.write   = &__k_fs_tmpfs_write;
	fsnode->fs.create  = &__k_fs_tmpfs_create;
	fsnode->fs.mkdir   = &__k_fs_tmpfs_mkdir;
	return fsnode;
}

static fs_node_t* __k_fs_tmpfs_create(fs_node_t* root, const char* path, uint8_t mode UNUSED) {
	tmpfs_node_t* dev   = root->device;
	tmpfs_node_t* child = __k_fs_tmpfs_create_node(path, dev);
	child->type |= TMPFS_TYPE_FILE;
	return __k_fs_tmpfs_to_fs_node(child);
}

static fs_node_t* __k_fs_tmpfs_mkdir(fs_node_t* root, const char* path, uint8_t mode UNUSED) {
	tmpfs_node_t* dev   = root->device;
	tmpfs_node_t* child = __k_fs_tmpfs_create_node(path, dev);
	child->type |= TMPFS_TYPE_DIR;
	return __k_fs_tmpfs_to_fs_node(child);
}

static fs_node_t* __k_fs_tmpfs_finddir(fs_node_t* root, const char* path) {
	tmpfs_node_t* tmp_node = root->device;
	for(uint32_t i = 0; i < tmp_node->children->size; i++) {
		tmpfs_node_t* child = tmp_node->children->data[i];
		if(!strcmp(child->name, path)) {
			return __k_fs_tmpfs_to_fs_node(child);
		}
	}
	return NULL;
}

static fs_node_t* __k_fs_tmpfs_create_root(char* name) {
	tmpfs_node_t* tmpfs_root = __k_fs_tmpfs_create_node(name, 0);
	tmpfs_root->type |= TMPFS_TYPE_DIR;
	return __k_fs_tmpfs_to_fs_node(tmpfs_root);
}

static fs_node_t* __k_fs_tmpfs_mount(const char* mountpoint, const char* device UNUSED) {
	char* fn = k_util_path_filename(mountpoint);
	fs_node_t* root = __k_fs_tmpfs_create_root(fn);
	k_free(fn);
	return root;
}

K_STATUS k_fs_tmpfs_init() {
	k_fs_vfs_register_fs("tmpfs", __k_fs_tmpfs_mount);
	return K_STATUS_OK;
}
