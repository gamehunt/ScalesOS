#include "dirent.h"
#include "mem/pmm.h"
#include "types/list.h"
#include "fs/vfs.h"
#include "mem/heap.h"
#include "mem/shm.h"

#include <string.h>

static list_t* __shm_mappings;

static shm_node_t* __k_mem_shm_create_shm_node(const char* name) {
	shm_node_t* shm = k_calloc(1, sizeof(shm_node_t));
	strncpy(shm->name, name, sizeof(shm->name));
	shm->frames = list_create();
	list_push_back(__shm_mappings, shm);
	return shm;
}

static void __k_mem_shm_open(fs_node_t* node, uint16_t flags UNUSED) {
	shm_node_t* sh = node->device;
	sh->links++;
}

static int __k_mem_shm_remove(fs_node_t* node) {
	shm_node_t* sh = node->device;
	for(size_t i = 0; i < sh->frames->size; i++) {
		k_mem_pmm_free((pmm_frame_t) sh->frames->data[i], 1);
	}
	list_delete_element(sh->root->device, sh);
	list_delete_element(__shm_mappings, sh);
	free(sh);
	return 0;
}

static void __k_mem_shm_close(fs_node_t* node) {
	shm_node_t* sh = node->device;
	sh->links--;
	if(sh->links <= 0) {
		__k_mem_shm_remove(node);
	}
}

static int __k_mem_shm_truncate(fs_node_t* node, off_t sz) {
	int32_t diff = sz - node->size;

	int dir = diff > 0;

	if(!dir) {
		diff = -diff;
	}

	off_t pages = diff / 0x1000;
	off_t bytes = diff % 0x1000;

	shm_node_t* shm = node->device;

	node->size = sz;

	if(dir) {
		for(uint32_t i = 0; i < pages + (bytes > 0); i++) {
			pmm_frame_t frame = k_mem_pmm_alloc_frames(1);
			list_push_back(shm->frames, (void*) frame);
		}
	} else {
		for(int32_t i = pages + (bytes > 0) - 1; i >= 0; i--) {
			pmm_frame_t frame = (pmm_frame_t) shm->frames->data[i];
			list_delete_element(shm->frames, (void*) frame);
			k_mem_pmm_free(frame, 1);
		}
	}

	return 0;
}

static fs_node_t* __k_mem_shm_create_node(shm_node_t* shm) {
	fs_node_t* node = k_fs_vfs_create_node(shm->name);

	node->device = shm;
	node->flags = VFS_FILE;
	node->fs.truncate = __k_mem_shm_truncate;
	node->fs.open     = __k_mem_shm_open;
	node->fs.close    = __k_mem_shm_close;

	return node;
}

static fs_node_t* __k_mem_shm_create(fs_node_t* root, const char* path, uint16_t flags UNUSED) {
	shm_node_t* shm = __k_mem_shm_create_shm_node(path);
	shm->root = root;
	list_t* par = root->device;
	list_push_back(par, shm);
	return __k_mem_shm_create_node(shm);
}

static struct dirent* __k_mem_shm_readdir(fs_node_t* dir, unsigned int index) {
	list_t* ls = dir->device;

	struct dirent* dent = k_malloc(sizeof(struct dirent));

	if(index == 0) {
		dent->ino = 1;
		strcpy(dent->name, ".");
		return dent;
	}

	if(index == 1) {
		dent->ino = 1;
		strcpy(dent->name, "..");
		return dent;
	}

	index -= 2;

	if(index < ls->size) {
		shm_node_t* node = ls->data[index];
		dent->ino = index;
		strncpy(dent->name, node->name, sizeof(dent->name));
		return dent;
	} else {
		free(dent);
		return NULL;
	}
}

static fs_node_t* __k_mem_shm_finddir(fs_node_t* root, const char* path) {
	list_t* ls = root->device;
	for(size_t i = 0; i < ls->size; i++) {
		shm_node_t* node = ls->data[i];
		if(!strcmp(node->name, path)) {
			return __k_mem_shm_create_node(node);	
		}
	}
	return NULL;
}


static fs_node_t* __k_mem_shm_create_root() {
	fs_node_t* node = k_fs_vfs_create_node("[shm-root]");

	node->device = list_create();
	node->flags = VFS_DIR;

	node->fs.readdir = __k_mem_shm_readdir;
	node->fs.finddir = __k_mem_shm_finddir;
	node->fs.create  = __k_mem_shm_create;

	return node;
}

void k_mem_shm_init() {
	__shm_mappings = list_create();
	k_fs_vfs_mount_node("/dev/shm", __k_mem_shm_create_root());
}

uint8_t k_mem_is_shm(fs_node_t* node) {
	return list_contains(__shm_mappings, node->device);
}
