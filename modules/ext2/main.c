#include "dirent.h"
#include <kernel/fs/vfs.h>
#include <kernel/mod/modules.h>
#include <kernel/mem/heap.h>
#include <kernel/mem/dma.h>
#include <kernel/kernel.h>
#include <kernel/util/log.h>
#include <kernel/util/path.h>
#include <kernel/util/perf.h>
#include <kernel/dev/timer.h>

#include <types/list.h>

#include <stdio.h>
#include <string.h>

#define EXT2_STATE_CLEAN 1
#define EXT2_STATE_ERROR 2

#define EXT2_EACT_IGNORE  1
#define EXT2_EACT_REMOUNT 2
#define EXT2_EACT_PANIC   3

#define EXT2_OPT_FEAT_PREALLOC (1 << 0)
#define EXT2_OPT_FEAT_AFS      (1 << 1)
#define EXT2_OPT_FEAT_JOURNAL  (1 << 2)
#define EXT2_OPT_FEAT_ATTR     (1 << 3)
#define EXT2_OPT_FEAT_RESIZE   (1 << 4)
#define EXT2_OPT_FEAT_HASH     (1 << 5)

#define EXT2_REQ_FEAT_COMPRESSION (1 << 0)
#define EXT2_REQ_FEAT_DIR_TYPE    (1 << 1)
#define EXT2_REQ_FEAT_REPLAY      (1 << 2)
#define EXT2_REQ_FEAT_JOURNAL_DEV (1 << 3)

#define EXT2_RDONLY_FEAT_SPARSE    (1 << 0)
#define EXT2_RDONLY_FEAT_SIZE64    (1 << 1)
#define EXT2_RDONLY_FEAT_DIR_BTREE (1 << 2)

#define EXT2_FIFO     0x1000
#define EXT2_CHARDEV  0x2000
#define EXT2_DIR      0x4000
#define EXT2_BLOCKDEV 0x6000
#define EXT2_FILE     0x8000
#define EXT2_SYMLINK  0xA000
#define EXT2_SOCKET   0xC000

#define EXT2_PERM_O_EXEC   0x001
#define EXT2_PERM_O_WRITE  0x002
#define EXT2_PERM_O_READ   0x004
#define EXT2_PERM_G_EXEC   0x008
#define EXT2_PERM_G_WRITE  0x010
#define EXT2_PERM_G_READ   0x020
#define EXT2_PERM_U_EXEC   0x040
#define EXT2_PERM_U_WRITE  0x080
#define EXT2_PERM_U_READ   0x100
#define EXT2_PERM_STICKY   0x200
#define EXT2_PERM_SETGUID  0x400
#define EXT2_PERM_SETUID   0x800

#define EXT2_INODE_FLAG_SECUREDELETION  (1 << 0)
#define EXT2_INODE_FLAG_KEEP            (1 << 1)
#define EXT2_INODE_FLAG_COMPRESSION     (1 << 2)
#define EXT2_INODE_FLAG_SYNC            (1 << 3)
#define EXT2_INODE_FLAG_IMMUTABLE       (1 << 4)
#define EXT2_INODE_FLAG_APPENDONLY      (1 << 5)
#define EXT2_INODE_FLAG_NODUMP          (1 << 6)
#define EXT2_INODE_FLAG_NOTIME          (1 << 7)
#define EXT2_INODE_FLAG_HASH            (1 << 16)
#define EXT2_INODE_FLAG_AFS             (1 << 17)
#define EXT2_INODE_FLAG_JOURNAL         (1 << 18)

#define EXT2_DIRENT_TYPE_UNK      0
#define EXT2_DIRENT_TYPE_FILE     1
#define EXT2_DIRENT_TYPE_DIR      2
#define EXT2_DIRENT_TYPE_CHARDEV  3
#define EXT2_DIRENT_TYPE_BLOCKDEV 4
#define EXT2_DIRENT_TYPE_FIFO     5
#define EXT2_DIRENT_TYPE_SOCKET   6
#define EXT2_DIRENT_TYPE_SYMLINK  7

#define BLOCK_SIZE(superblock)    (1024 << superblock->block_size)
#define FRAGMENT_SIZE(superblock) (1024 << superblock->fragment_size)

#define GROUP_FROM_INODE(superblock, inode) ((inode - 1) / superblock->inodes_per_group)
#define INDEX_FROM_INODE(superblock, inode) ((inode - 1) % superblock->inodes_per_group)
#define BLOCK_FROM_INDEX(superblock, index) ((index * superblock->inode_size) / BLOCK_SIZE(superblock))

#define DIRECT_BLOCK_CAPACITY 12
#define SINGLE_INDIRECT_BLOCK_CAPACITY(superblock) (BLOCK_SIZE(superblock) / sizeof(uint32_t))
#define DOUBLE_INDIRECT_BLOCK_CAPACITY(superblock) (BLOCK_SIZE(superblock) / sizeof(uint32_t)) * SINGLE_INDIRECT_BLOCK_CAPACITY(superblock)
#define TRIPLE_INDIRECT_BLOCK_CAPACITY(superblock) (BLOCK_SIZE(superblock) / sizeof(uint32_t)) * DOUBLE_INDIRECT_BLOCK_CAPACITY(superblock)

#define INODES_PER_CACHE 256
#define BLOCKS_PER_CACHE 2048

#define MAX_READAHEAD MB(8)

typedef struct {
	uint32_t total_inodes;
	uint32_t total_blocks;
	uint32_t reserved_blocks;
	uint32_t free_block;
	uint32_t free_inodes;
	uint32_t superblock;
	uint32_t block_size;
	uint32_t fragment_size;
	uint32_t blocks_per_group;
	uint32_t fragments_per_group;
	uint32_t inodes_per_group;
	uint32_t last_mount;
	uint32_t last_write;
	uint16_t mounts_from_fsck;
	uint16_t max_mounts_before_fsck;
	uint16_t signature;
	uint16_t state;
	uint16_t error_action;
	uint16_t version_minor;
	uint32_t last_fsck;
	uint32_t fsck_interval;
	uint32_t os;
	uint32_t version_major;
	uint16_t reserved_uid;
	uint16_t reserved_guid;

	//Extended
	uint32_t first_free_inode;
	uint16_t inode_size;
    uint16_t superblock_group;
	uint32_t optional_features;
	uint32_t required_features;
	uint32_t readonly_features;
	char     uuid[16];
	char     volume_name[16];
	char     last_path[64];
	uint32_t compression;
	uint8_t  files_prealloc;
	uint8_t  dirs_prealloc;
	uint16_t unused;
	char     journal_id[16];
	uint32_t journal_inode;
	uint32_t journal_dev;
	uint32_t orphan_inodes_list;
	uint8_t  pad[788];
} __attribute__((packed)) ext2_superblock_t;

typedef struct {
	uint32_t blocks_bitmap;
	uint32_t inode_bitmap;
	uint32_t inode_table;
	uint16_t free_blocks;
	uint16_t free_inodes;
	uint16_t directories;
	uint8_t  pad[14];
} __attribute__((packed)) ext2_block_group_descriptor_t;

typedef struct {
	uint16_t perms;
	uint16_t uid;
	uint32_t size_low;
	uint32_t access_time;
	uint32_t create_time;
	uint32_t modification_time;
	uint32_t deletion_time;
	uint16_t guid;
	uint16_t hard_links;
	uint32_t sectors;
	uint32_t flags;
	uint32_t os1;
	uint32_t block_pointers[DIRECT_BLOCK_CAPACITY];
	uint32_t indirect_block_pointer_s;
	uint32_t indirect_block_pointer_d;
	uint32_t indirect_block_pointer_t;
	uint32_t generation;
	uint32_t extended_attribute_block;
	uint32_t size_upper;
	uint32_t fragment;
	uint32_t os2[3];
} __attribute__((packed)) ext2_inode_t;

typedef struct {
	uint32_t inode;
	uint16_t size;
	uint8_t  name_length;
	uint8_t  type;
	char     name[255];
} __attribute__((packed)) ext2_dirent_t;

typedef struct {
	uint32_t      inode_number;
	ext2_inode_t* inode;
} cached_inode_t;

typedef struct {
	uint32_t  block_number;
	uint32_t  last_access;
	void*     block;
} cached_block_t;

typedef struct {
	ext2_superblock_t*             superblock;
	fs_node_t*                     device;
	ext2_block_group_descriptor_t* bgds;
	size_t                         bgds_amount;
	list_t*                        inode_cache;
	list_t*                        block_cache;
	void*                          readahead_buffer;
} ext2_fs_t;

ext2_inode_t* __ext2_try_get_cached_inode(ext2_fs_t* fs, uint32_t inode) {
	LOCK(fs->inode_cache->lock)
	for(size_t i = 0; i < fs->inode_cache->size; i++) {
		cached_inode_t* c_ino = fs->inode_cache->data[i];
		if(c_ino->inode_number == inode) {
			c_ino->inode->access_time = now();
			ext2_inode_t* ino = k_malloc(fs->superblock->inode_size);
			memcpy(ino, c_ino->inode, fs->superblock->inode_size);
			list_delete_element(fs->inode_cache, c_ino);
			list_push_front(fs->inode_cache, c_ino);
			UNLOCK(fs->inode_cache->lock)
			return ino;
		}
	}
	UNLOCK(fs->inode_cache->lock)
	return NULL;
}

void* __ext2_try_get_cached_block(ext2_fs_t* fs, uint32_t block) {
	LOCK(fs->block_cache->lock)
	for(size_t i = 0; i < fs->block_cache->size; i++) {
		cached_block_t* c_bl = fs->block_cache->data[i];
		if(c_bl->block_number == block) {
			c_bl->last_access = now();
			list_delete_element(fs->block_cache, c_bl);
			list_push_front(fs->block_cache, c_bl);
			UNLOCK(fs->block_cache->lock)
			return c_bl->block;
		}
	}
	UNLOCK(fs->block_cache->lock)
	return NULL;
}

void __ext2_invalidate_block(ext2_fs_t* fs, uint32_t block) {
	LOCK(fs->block_cache->lock)
	for(size_t i = 0; i < fs->block_cache->size; i++) {
		cached_block_t* c_bl = fs->block_cache->data[i];
		if(c_bl->block_number == block) {
			list_delete_element(fs->block_cache, c_bl);
			break;
		}
	}
	UNLOCK(fs->block_cache->lock)
}

void __ext2_cache_inode(ext2_fs_t* fs, uint32_t num, ext2_inode_t* inode) {
	LOCK(fs->inode_cache->lock)
	cached_inode_t* copy = k_malloc(sizeof(cached_inode_t));
	copy->inode_number = num; 
	copy->inode = k_malloc(fs->superblock->inode_size);
	memcpy(copy->inode, inode, fs->superblock->inode_size);
	copy->inode->access_time = now();
	list_push_front(fs->inode_cache, copy);
	if(fs->inode_cache->size > INODES_PER_CACHE) {
		cached_inode_t* last = list_pop_back(fs->inode_cache);
		k_free(last->inode);
		k_free(last);
	} 
	UNLOCK(fs->inode_cache->lock)
}

void __ext2_invalidate_inode(ext2_fs_t* fs, uint32_t inode) {
	LOCK(fs->inode_cache->lock)
	for(size_t i = 0; i < fs->inode_cache->size; i++) {
		cached_inode_t* c_ino = fs->inode_cache->data[i];
		if(c_ino->inode_number == inode) {
			list_delete_element(fs->inode_cache, c_ino);
			break;
		}
	}
	UNLOCK(fs->inode_cache->lock)
}

void __ext2_cache_block(ext2_fs_t* fs, uint32_t block, void* data) {
	LOCK(fs->block_cache->lock)
	cached_block_t* copy = k_malloc(sizeof(cached_block_t));
	copy->block_number = block; 
	copy->block = k_malloc(BLOCK_SIZE(fs->superblock));
	memcpy(copy->block, data, BLOCK_SIZE(fs->superblock));
	copy->last_access = now();
	list_push_front(fs->block_cache, copy);
	if(fs->block_cache->size > BLOCKS_PER_CACHE) {
		cached_block_t* last = list_pop_back(fs->block_cache);
		k_free(last->block);
		k_free(last);
	} 
	UNLOCK(fs->block_cache->lock)
}

static uint32_t __ext2_read_block(ext2_fs_t* fs, uint32_t start_block, uint8_t* buffer, uint32_t readahead) {
	if(start_block > fs->superblock->total_blocks) {
		k_warn("ext2_read_block(): tried to read invalid block %d", start_block);
		return 0;
	}
	uint32_t block_size = BLOCK_SIZE(fs->superblock);
	void* bl = __ext2_try_get_cached_block(fs, start_block);
	if(bl) {
		memcpy(buffer, bl, block_size);
		return block_size; 
	} else {
		if(readahead) {
			if(readahead > MAX_READAHEAD) {
				readahead = MAX_READAHEAD;
			}
			uint32_t read = k_fs_vfs_read(fs->device, start_block * block_size, readahead, fs->readahead_buffer);	
			for(size_t i = 0; i < read; i += block_size) {
				__ext2_cache_block(fs, start_block + i / block_size, fs->readahead_buffer + i);
			}
			memcpy(buffer, fs->readahead_buffer, block_size);
			return block_size;
		} else {
			uint32_t read = k_fs_vfs_read(fs->device, start_block * block_size, block_size, buffer);
			if(read == block_size) {
				__ext2_cache_block(fs, start_block, buffer);
			}
			return read;
		}
	}
}

static uint32_t __ext2_resolve_single_indirect_block(ext2_fs_t* fs, uint32_t pointer, uint32_t block_offset) {
	if(!pointer) {
		return 0;
	}
	void* block_buffer = k_malloc(BLOCK_SIZE(fs->superblock));
	__ext2_read_block(fs, pointer, block_buffer, 0);
	uint32_t target_block = ((uint32_t*) block_buffer) [block_offset - (DIRECT_BLOCK_CAPACITY)]; 
	k_free(block_buffer);
	return target_block;
}

static uint32_t __ext2_resolve_double_indirect_block(ext2_fs_t* fs, uint32_t pointer, uint32_t block_offset) {
	if(!pointer) {
		return 0;
	}
	uint32_t extra_offset = block_offset - (SINGLE_INDIRECT_BLOCK_CAPACITY(fs->superblock));
	void* block_buffer = k_malloc(BLOCK_SIZE(fs->superblock));
	__ext2_read_block(fs, pointer, block_buffer, 0);
	uint32_t target_block = ((uint32_t*) block_buffer) [extra_offset / SINGLE_INDIRECT_BLOCK_CAPACITY(fs->superblock)]; 
	k_free(block_buffer);
	return __ext2_resolve_single_indirect_block(fs, target_block, extra_offset % SINGLE_INDIRECT_BLOCK_CAPACITY(fs->superblock));
}

static uint32_t __ext2_resolve_triple_indirect_block(ext2_fs_t* fs, uint32_t pointer, uint32_t block_offset) {
	if(!pointer) {
		return 0;
	}
	uint32_t extra_offset = block_offset - (DOUBLE_INDIRECT_BLOCK_CAPACITY(fs->superblock));
	void* block_buffer = k_malloc(BLOCK_SIZE(fs->superblock));
	__ext2_read_block(fs, pointer, block_buffer, 0);
	uint32_t target_block = ((uint32_t*) block_buffer) [extra_offset / DOUBLE_INDIRECT_BLOCK_CAPACITY(fs->superblock)]; 
	k_free(block_buffer);
	return __ext2_resolve_double_indirect_block(fs, target_block, extra_offset % DOUBLE_INDIRECT_BLOCK_CAPACITY(fs->superblock));
}

static uint32_t __ext2_block_from_offset(ext2_fs_t* fs, ext2_inode_t* inode, uint32_t block_offset) {
		if(block_offset < DIRECT_BLOCK_CAPACITY) {
			return inode->block_pointers[block_offset];
		} else if (block_offset < SINGLE_INDIRECT_BLOCK_CAPACITY(fs->superblock) && inode->indirect_block_pointer_s){
			return __ext2_resolve_single_indirect_block(fs, inode->indirect_block_pointer_s, block_offset);
		} else if(block_offset < DOUBLE_INDIRECT_BLOCK_CAPACITY(fs->superblock) && inode->indirect_block_pointer_d) {
			return __ext2_resolve_double_indirect_block(fs, inode->indirect_block_pointer_d, block_offset);
		} else if(block_offset < TRIPLE_INDIRECT_BLOCK_CAPACITY(fs->superblock) && inode->indirect_block_pointer_t) {
			return __ext2_resolve_triple_indirect_block(fs, inode->indirect_block_pointer_t, block_offset);
		} else {
			return 0;
		}
}

static uint32_t __ext2_read_inode_contents(ext2_fs_t* fs, ext2_inode_t* inode, uint32_t offset, uint32_t size, uint8_t* buffer) {
	if(offset >= inode->size_low) {
		return 0;
	}

	if(offset + size >= inode->size_low) {
		size = inode->size_low - offset;
		if(!size) {
			return 0;
		}
	}

	uint32_t block_size = BLOCK_SIZE(fs->superblock);
	uint32_t block_offset = offset / block_size;
	uint32_t part_offset  = offset % block_size;

	void* block_buffer = k_malloc(block_size);

	uint32_t buffer_offset = 0;

	while(size > 0) {
		uint32_t target_block = __ext2_block_from_offset(fs, inode, block_offset);
		if(!target_block) {
			break;
		}

		__ext2_read_block(fs, target_block, block_buffer, (inode->size_low / block_size - block_offset + 1) * block_size);
		
		if(size <= block_size - part_offset) {
			memcpy(&buffer[buffer_offset], &block_buffer[part_offset], size);
			buffer_offset += size;
			break;
		} else{
			memcpy(&buffer[buffer_offset], &block_buffer[part_offset], block_size - part_offset);
			size -= (block_size - part_offset);
			buffer_offset += (block_size - part_offset);
			part_offset = 0;
			block_offset++;
		}
	}

	k_free(block_buffer);
	return buffer_offset;
}


static ext2_inode_t* __ext2_read_inode(ext2_fs_t* fs, uint32_t inode) {
	if(!inode) {
		return NULL;
	}

	ext2_inode_t* cached = __ext2_try_get_cached_inode(fs, inode);
	if(cached) {
		return cached;
	}

	uint32_t block_size = BLOCK_SIZE(fs->superblock);

	void* buffer = k_malloc(block_size);

	uint32_t group = GROUP_FROM_INODE(fs->superblock, inode);

	ext2_block_group_descriptor_t* block_group_table = fs->bgds;
	uint32_t inode_table  = block_group_table[group].inode_table;

	uint32_t inode_index  = INDEX_FROM_INODE(fs->superblock, inode);
	uint32_t block_offset = BLOCK_FROM_INDEX(fs->superblock, inode_index);
	uint32_t inner_offset = inode_index - block_offset * (block_size / fs->superblock->inode_size);
	uint32_t target_block = inode_table + block_offset;

	__ext2_read_block(fs, target_block, buffer, 0);

	ext2_inode_t* actual_inode = k_malloc(fs->superblock->inode_size);

	memcpy(actual_inode, (uint8_t*) (((uintptr_t)buffer) + inner_offset * fs->superblock->inode_size), fs->superblock->inode_size);

	__ext2_cache_inode(fs, inode, actual_inode);

	k_free(buffer);

	return actual_inode;
}

static uint32_t __ext2_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
	if(!size) {
		return 0;
	}

	ext2_fs_t* fs = node->device;
	if(!fs) {
		return -1;
	}

	ext2_inode_t* inode = __ext2_read_inode(fs, node->inode);
	if(!inode) {
		return -1;
	}

	uint32_t read = __ext2_read_inode_contents(fs, inode, offset, size, buffer);

	k_free(inode);

	return read;
}

static struct dirent* __ext2_readdir(fs_node_t* dir, uint32_t index) {
	ext2_fs_t* fs = dir->device;
	if(!fs) {
		return 0;
	}

	ext2_inode_t* inode = __ext2_read_inode(fs, dir->inode);

	if(!inode){
		k_info("%s (%d): invalid inode.", dir->name, dir->inode);
		return 0;
	}

	if(!(inode->perms & EXT2_DIR)) {
		k_info("%s (%d): not a directory (0x%x)", dir->name, dir->inode, inode->perms);
		return 0;
	}

	ext2_dirent_t  dirent;
	
	uint32_t offset = 0;
	uint32_t cur_index = 0;

	while(1) {
		if(offset >= inode->size_low) {
			return 0;
		}

		__ext2_read_inode_contents(fs, inode, offset, sizeof(ext2_dirent_t), (uint8_t*) &dirent);
		
		if(dirent.inode && index == cur_index) {
			break;
		}

		if(dirent.inode) {
			cur_index++;
		}
		
		offset += dirent.size;
	}

	struct dirent* result = k_malloc(sizeof(struct dirent));

	memcpy(result->name, dirent.name, dirent.name_length);
	result->name[dirent.name_length] = '\0';
	result->ino = dirent.inode;

	return result;
}

static fs_node_t* __ext2_finddir(fs_node_t* node, const char* path);
static uint32_t   __ext2_write(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);

static fs_node_t* __ext2_from_inode(ext2_fs_t* fs, const char* name, uint32_t inode) {
	ext2_inode_t* ino = __ext2_read_inode(fs, inode);
	fs_node_t* file = k_fs_vfs_create_node(name);
	strcpy(file->name, name);
	file->inode = inode;
	file->device = fs;
	file->size = ino->size_low; 
	
	if(ino->perms & EXT2_DIR) {
		file->flags |= VFS_DIR;
	} else {
		file->flags |= VFS_FILE;
	}

	file->fs.read    = &__ext2_read;
	file->fs.write   = &__ext2_write;
	file->fs.readdir = &__ext2_readdir;
	file->fs.finddir = &__ext2_finddir;
	k_free(ino);
	return file;
}

static fs_node_t* __ext2_finddir(fs_node_t* root, const char* path) {
	uint32_t segments = k_util_path_length(path);
	for(uint32_t i = 0; i < segments; i++) {
		char* folder = k_util_path_segment(path, i);
		struct dirent* d = 0;
		uint32_t index = 0;
		uint8_t found = 0;
		do {
			d = __ext2_readdir(root, index);
			if(!d) {
				break;
			}
			if(!strcmp(d->name, folder)) {
				root = __ext2_from_inode(root->device, d->name, d->ino);
				k_free(d);
				found = 1;
				break;
			}
			k_free(d);
			index++;
		} while(d);
		k_free(folder);
		if(!found) {
			return 0;
		}
	}
	char* final = k_util_path_filename(path);
	if(strcmp(root->name, final)) {
		k_free(final);
		return 0;
	}
	k_free(final);
	return root;
}

static void __ext2_write_superblock(ext2_fs_t* fs) {
	k_fs_vfs_write(fs->device, 1024, 1024, fs);
}

static void __ext2_write_inode(ext2_fs_t* fs, uint32_t inode, ext2_inode_t* inode_data) {
	uint32_t block_size = BLOCK_SIZE(fs->superblock);

	uint32_t group = GROUP_FROM_INODE(fs->superblock, inode);

	ext2_block_group_descriptor_t* block_group_table = fs->bgds;
	uint32_t inode_table  = block_group_table[group].inode_table;

	uint32_t inode_index  = INDEX_FROM_INODE(fs->superblock, inode);
	uint32_t block_offset = BLOCK_FROM_INDEX(fs->superblock, inode_index);
	uint32_t inner_offset = inode_index - block_offset * (block_size / fs->superblock->inode_size);
	uint32_t target_block = inode_table + block_offset;

	k_fs_vfs_write(fs->device, target_block * block_size + inner_offset * fs->superblock->inode_size, fs->superblock->inode_size, inode_data);

	__ext2_invalidate_inode(fs, inode);
}

static uint32_t* __ext2_free_block_slot_from_inode(ext2_inode_t* inode) {
	for(int i = 0; i < DIRECT_BLOCK_CAPACITY; i++) {
		if(!inode->block_pointers[i]) {
			return &inode->block_pointers[i];
		}
	}
	// TODO
	if(inode->indirect_block_pointer_s) {
		
	}
	if(inode->indirect_block_pointer_d) {

	}
	if(inode->indirect_block_pointer_t) {

	}

	return 0;
}

static uint32_t __ext2_get_free_block_from_bitmap(uint32_t* bitmap) {
	for(uint32_t i = 0; i < 1024; i++) {
		if(bitmap[i] == 0xFFFFFFFF) {
			continue;
		}
		for(uint32_t j = 0; j < 32; j++) {
			if(!(bitmap[i] & (1 << j))) {
				bitmap[i] |= (1 << j);
				return i * 32 + j;
			}
		}
	}
	return 0;
}

static void __ext2_allocate_blocks_for_inode(ext2_fs_t* fs, uint32_t inode, ext2_inode_t* inode_data, uint32_t blocks) {
	uint32_t block_size = BLOCK_SIZE(fs->superblock);
	uint32_t group      = GROUP_FROM_INODE(fs->superblock, inode);

	ext2_block_group_descriptor_t desc = fs->bgds[group];

	if(desc.free_blocks >= blocks) {
		uint32_t  bitmap_address = desc.blocks_bitmap;	
		uint32_t* bitmap = k_malloc(block_size);
		uint32_t  block_offset = group * fs->superblock->blocks_per_group;

		__ext2_read_block(fs, bitmap_address, bitmap, 0);

		while(blocks) {
			*__ext2_free_block_slot_from_inode(inode_data) = block_offset + __ext2_get_free_block_from_bitmap(bitmap);
			blocks--;
		}

		k_fs_vfs_write(fs->device, bitmap_address, block_size, bitmap);

		k_free(bitmap);
	}
}

static uint32_t __ext2_count_allocated_blocks(ext2_inode_t* ino) {
	uint32_t t = 0;
	for(int i = 0; i < DIRECT_BLOCK_CAPACITY; i++) {
		if(ino->block_pointers[i]) {
			t++;
		} else {
			break;
		}
	}
	return t;
}

static uint32_t __ext2_write(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
	if(offset >= node->size) {
		return 0;
	}

	ext2_fs_t*     fs   = node->device;
	ext2_inode_t*  ino  = __ext2_read_inode(fs, node->inode);

	if(!ino) {
		return 0;
	}

	uint32_t block_size = BLOCK_SIZE(fs->superblock);
	uint32_t block_offset = offset / block_size;
	uint32_t part_offset  = offset % block_size;

	uint32_t blocks_needed 	     = size / block_size;
	uint32_t part_needed_before  = block_size - part_offset;
	if(part_needed_before == block_size) {
		part_needed_before = 0;
	}
	uint32_t part_needed_after   = size - blocks_needed - part_needed_before;

	uint32_t total_blocks = blocks_needed + (part_needed_after > 0) + (part_needed_before > 0);
	uint32_t total_offset = block_offset - (part_offset > 0);

	uint32_t allocated_blocks = __ext2_count_allocated_blocks(ino);

	if(blocks_needed > allocated_blocks) {
		uint32_t delta = blocks_needed - allocated_blocks;	
		__ext2_allocate_blocks_for_inode(fs, node->inode, ino, delta);
	}

	void* internal_buffer = k_malloc(total_blocks * block_size);
	__ext2_read_inode_contents(fs, ino, total_offset * block_size, total_blocks * block_size, internal_buffer);
	memcpy(internal_buffer + part_offset, buffer, size);
	uint32_t r = k_fs_vfs_write(fs->device, total_offset * block_size, total_blocks * block_size, internal_buffer);

	__ext2_write_inode(fs, node->inode, ino);

	k_free(ino);
	k_free(internal_buffer);

	return r;
}

static fs_node_t* __ext2_mount(const char* path, const char* device) {
	fs_node_t* dev = k_fs_vfs_open(device, O_RDWR);

	ext2_superblock_t* superblock = k_malloc(1024);

	k_fs_vfs_read(dev, 1024, 1024, (uint8_t*) superblock);

	if(superblock->signature != 0xef53) {
		k_err("Not an ext2: %s", device);
		k_fs_vfs_close(dev);
		return 0;
	}

	ext2_fs_t* fs = k_malloc(sizeof(ext2_fs_t));

	fs->superblock       = superblock;
	fs->device           = dev;
	fs->inode_cache      = list_create();
	fs->block_cache      = list_create();
	fs->readahead_buffer = k_malloc(MAX_READAHEAD);

	fs->bgds_amount = fs->superblock->total_blocks / fs->superblock->blocks_per_group;
	while(fs->bgds_amount * fs->superblock->blocks_per_group < fs->superblock->total_blocks) {
		fs->bgds_amount++;
	}

	uint32_t block_amount = sizeof(ext2_block_group_descriptor_t) * fs->bgds_amount / fs->superblock->total_blocks + 1;
	fs->bgds = k_malloc(block_amount * BLOCK_SIZE(fs->superblock));

	uint32_t start = 2;
	if(BLOCK_SIZE(fs->superblock) > 1024) {
		start = 1;
	}

	for(uint32_t i = 0; i < block_amount; i++) {
		__ext2_read_block(fs, start + i, (void*) ((uint32_t)fs->bgds + BLOCK_SIZE(fs->superblock) * i), 0);
	}

	char* filename = k_util_path_filename(path);

	fs_node_t* root = k_fs_vfs_create_node(filename);
	root->inode  = 2;
	root->device = fs;
	root->flags  = VFS_DIR;
	root->fs.write   = &__ext2_write;
	root->fs.read    = &__ext2_read;
	root->fs.readdir = &__ext2_readdir;
	root->fs.finddir = &__ext2_finddir;

	k_free(filename);

	k_debug("EXT2 block size: %d", BLOCK_SIZE(fs->superblock));

	return root;
}

K_STATUS load(){
	k_fs_vfs_register_fs("ext2", __ext2_mount);
    return K_STATUS_OK;
}

K_STATUS unload(){
    return K_STATUS_OK;
}

MODULE("ext2", &load, &unload)
