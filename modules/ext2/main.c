#include "dirent.h"
#include <kernel/fs/vfs.h>
#include <kernel/mod/modules.h>
#include <kernel/mem/heap.h>
#include <kernel/kernel.h>
#include <kernel/util/log.h>
#include <kernel/util/path.h>

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
	ext2_superblock_t* superblock;
	fs_node_t*         device;
} ext2_fs_t;


static uint32_t __ext2_read_block(ext2_fs_t* fs, uint32_t block, uint8_t* buffer) {
	if(block > fs->superblock->total_blocks) {
		k_warn("ext2_read_block(): tried to read invalid block %d", block);
		return 0;
	}
	uint32_t read = k_fs_vfs_read(fs->device, block * BLOCK_SIZE(fs->superblock), BLOCK_SIZE(fs->superblock), buffer);	
	if(read != (uint32_t) BLOCK_SIZE(fs->superblock)) {
		k_warn("ext2_read_block(): partial read. Read only %d/%d bytes of block %d", read, BLOCK_SIZE(fs->superblock), block);
	}
	return read;
}

static uint32_t __ext2_resolve_single_indirect_block(ext2_fs_t* fs, uint32_t pointer, uint32_t block_offset) {
	if(!pointer) {
		return 0;
	}
	void* block_buffer = k_malloc(BLOCK_SIZE(fs->superblock));
	__ext2_read_block(fs, pointer, block_buffer);
	uint32_t target_block = ((uint32_t*) block_buffer) [block_offset - (DIRECT_BLOCK_CAPACITY + 1)]; 
	k_free(block_buffer);
	return target_block;
}

static uint32_t __ext2_resolve_double_indirect_block(ext2_fs_t* fs, uint32_t pointer, uint32_t block_offset) {
	if(!pointer) {
		return 0;
	}
	uint32_t extra_offset = block_offset - (SINGLE_INDIRECT_BLOCK_CAPACITY(fs->superblock) + 1);
	void* block_buffer = k_malloc(BLOCK_SIZE(fs->superblock));
	__ext2_read_block(fs, pointer, block_buffer);
	uint32_t target_block = ((uint32_t*) block_buffer) [extra_offset / SINGLE_INDIRECT_BLOCK_CAPACITY(fs->superblock)]; 
	k_free(block_buffer);
	return __ext2_resolve_single_indirect_block(fs, target_block, extra_offset % SINGLE_INDIRECT_BLOCK_CAPACITY(fs->superblock));
}

static uint32_t __ext2_resolve_triple_indirect_block(ext2_fs_t* fs, uint32_t pointer, uint32_t block_offset) {
	if(!pointer) {
		return 0;
	}
	uint32_t extra_offset = block_offset - (DOUBLE_INDIRECT_BLOCK_CAPACITY(fs->superblock) + 1);
	void* block_buffer = k_malloc(BLOCK_SIZE(fs->superblock));
	__ext2_read_block(fs, pointer, block_buffer);
	uint32_t target_block = ((uint32_t*) block_buffer) [extra_offset / DOUBLE_INDIRECT_BLOCK_CAPACITY(fs->superblock)]; 
	k_free(block_buffer);
	return __ext2_resolve_double_indirect_block(fs, target_block, extra_offset % DOUBLE_INDIRECT_BLOCK_CAPACITY(fs->superblock));
}

static uint32_t __ext2_read_inode_contents(ext2_fs_t* fs, ext2_inode_t* inode, uint32_t offset, uint32_t size, uint8_t* buffer) {
	if(offset >= inode->size_low) {
		return 0;
	}

	if(offset + size >= inode->size_low) {
		size = inode->size_low - offset;
	}

	uint32_t block_size = BLOCK_SIZE(fs->superblock);
	uint32_t block_offset = offset / block_size;
	uint32_t part_offset  = offset % block_size;

	void* block_buffer = k_malloc(block_size);

	uint32_t buffer_offset = 0;

	while(size > 0) {
		uint32_t target_block = 0;
		if(block_offset < DIRECT_BLOCK_CAPACITY) {
			target_block = inode->block_pointers[block_offset];
		} else if (block_offset < SINGLE_INDIRECT_BLOCK_CAPACITY(fs->superblock) && inode->indirect_block_pointer_s){
			target_block = __ext2_resolve_single_indirect_block(fs, inode->indirect_block_pointer_s, block_offset);
		} else if(block_offset < DOUBLE_INDIRECT_BLOCK_CAPACITY(fs->superblock) && inode->indirect_block_pointer_d) {
			target_block = __ext2_resolve_double_indirect_block(fs, inode->indirect_block_pointer_d, block_offset);
		} else if(block_offset < TRIPLE_INDIRECT_BLOCK_CAPACITY(fs->superblock) && inode->indirect_block_pointer_t) {
			target_block = __ext2_resolve_triple_indirect_block(fs, inode->indirect_block_pointer_t, block_offset);
		} 

		if(!target_block) {
			return buffer_offset;	
		}

		__ext2_read_block(fs, target_block, block_buffer);
		
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
		return 0;
	}

	uint32_t block_size = BLOCK_SIZE(fs->superblock);

	void* buffer = k_malloc(block_size);

	uint32_t group = GROUP_FROM_INODE(fs->superblock, inode);
	uint32_t table_block_offset = group * sizeof(ext2_block_group_descriptor_t) / block_size;
	uint32_t table_block = (BLOCK_SIZE(fs->superblock) == 1024 ? 2 : 1) + table_block_offset; 
	uint32_t shifted_group = group - table_block_offset * block_size / sizeof(ext2_block_group_descriptor_t);
	__ext2_read_block(fs, table_block, buffer);

	ext2_block_group_descriptor_t* block_group_table = buffer;
	uint32_t inode_table  = block_group_table[shifted_group].inode_table;

	uint32_t inode_index  = INDEX_FROM_INODE(fs->superblock, inode);
	uint32_t block_offset = BLOCK_FROM_INDEX(fs->superblock, inode_index);
	uint32_t inner_offset = inode_index - block_offset * (block_size / fs->superblock->inode_size);
	uint32_t target_block = inode_table + block_offset;

	__ext2_read_block(fs, target_block, buffer);

	ext2_inode_t* actual_inode = k_malloc(fs->superblock->inode_size);

	memcpy(actual_inode, (uint8_t*) (((uintptr_t)buffer) + inner_offset * fs->superblock->inode_size), fs->superblock->inode_size);

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

	k_free(inode);

	return result;
}

static fs_node_t* __ext2_finddir(fs_node_t* node, const char* path);

static fs_node_t* __ext2_from_inode(ext2_fs_t* fs, const char* name, uint32_t inode) {
	ext2_inode_t* ino = __ext2_read_inode(fs, inode);
	fs_node_t* file = k_fs_vfs_create_node(name);
	strcpy(file->name, name);
	file->inode = inode;
	file->device = fs;
	file->size = ino->size_low; 
	file->fs.read    = &__ext2_read;
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

	fs->superblock = superblock;
	fs->device     = dev;

	char* filename = k_util_path_filename(path);

	fs_node_t* root = k_fs_vfs_create_node(filename);
	root->inode  = 2;
	root->device = fs;
	root->fs.read    = &__ext2_read;
	root->fs.readdir = &__ext2_readdir;
	root->fs.finddir = &__ext2_finddir;

	k_free(filename);

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
