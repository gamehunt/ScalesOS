#include <kernel/fs/vfs.h>
#include <kernel/mod/modules.h>
#include <kernel/kernel.h>
#include <kernel/util/log.h>
#include <kernel/mem/heap.h>
#include <stdio.h>
#include <string.h>

typedef struct {
	uint8_t  attributes;
	uint8_t  chs_start[3];
	uint8_t  type;
	uint8_t  chs_end[3];
	uint32_t lba_start;
	uint32_t size;
} mbr_patrition_t;

typedef struct {
	uint8_t  code[440];
	char     uuid[4];
	uint16_t reserved;
	mbr_patrition_t patritions[4];
	uint16_t signature;
} __attribute__((packed)) mbr_t;

static uint32_t __dospart_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
	fs_node_t* drive = node->device;
	mbr_patrition_t* patrition_info = (mbr_patrition_t*) node->inode;

	uint32_t patr_offset = patrition_info->lba_start * 512;
	uint32_t patr_size   = patrition_info->size * 512;

	if(offset >= patr_offset + patr_size) {
		return 0;
	}

	if(offset + size >= patr_offset + patr_size) {
		size = patr_offset + patr_size - offset;
		if(!size) {
			return 0;
		}
	}

	return k_fs_vfs_read(drive, patr_offset + offset, size, buffer);
}

static uint32_t __dospart_write(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
	fs_node_t* drive = node->device;
	mbr_patrition_t* patrition_info = (mbr_patrition_t*) node->inode;

	uint32_t patr_offset = patrition_info->lba_start * 512;
	uint32_t patr_size   = patrition_info->size * 512;

	if(offset >= patr_offset + patr_size) {
		return 0;
	}

	if(offset + size >= patr_offset + patr_size) {
		size = patr_offset + patr_size - offset;
		if(!size) {
			return 0;
		}
	}

	return k_fs_vfs_write(drive, patr_offset + offset, size, buffer);
}

static fs_node_t* __dospart_create_device(fs_node_t* device, mbr_patrition_t* patr, int i) {
	char name[64];
	sprintf(name, "%s%d", device->name, i);

	mbr_patrition_t* patr_copy = k_malloc(sizeof(mbr_patrition_t));
	memcpy(patr_copy, patr, sizeof(mbr_patrition_t));

	fs_node_t* node = k_fs_vfs_create_node(name);
	node->inode = (uint32_t) patr_copy;
	node->device = device;
	node->size = patr->size;
	node->fs.read  = &__dospart_read;
	node->fs.write = &__dospart_write;

	return node;
}

static void __dospart_try_mbr(fs_node_t* device) {
	mbr_t mbr;

	k_fs_vfs_read(device, 0, 512, (uint8_t*) &mbr);

	if(mbr.signature != 0xaa55) {
		k_info("%s: not an mbr. Signature: 0x%x", device->name, mbr.signature);
		return;
	}

	uint8_t found = 0;

	for(int i = 0; i < 4; i++) {
		if(!mbr.patritions[i].lba_start) {
			continue;
		}

		found = 1;

		k_info("MBR Patrition: %ld - %ld (%d)", mbr.patritions[i].lba_start, 
				mbr.patritions[i].lba_start + mbr.patritions[i].size, 
				mbr.patritions[i].type);

		fs_node_t* node = __dospart_create_device(device, &mbr.patritions[i], i);
		
		char path[64];
		sprintf(path, "/dev/%s", node->name);

		k_fs_vfs_mount_node(path, node);

		k_info("Mounted as %s", path);
	}

	if(!found) {
		k_fs_vfs_close(device);
	}
}

K_STATUS load(){
	for(char l = 'a'; l <= 'z'; l++) {
		char path[64];
		sprintf(path, "/dev/hd%c", l);
		fs_node_t* node = k_fs_vfs_open(path, O_RDONLY); 
		if(node) {
			__dospart_try_mbr(node);
		} 
	}

    return K_STATUS_OK;
}

K_STATUS unload(){
    return K_STATUS_OK;
}

MODULE("dospart", &load, &unload);
MODULE_DEPENDENCIES("pre:ata");
