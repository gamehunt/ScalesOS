#ifndef __K_MEM_MMAP_H
#define __K_MEM_MMAP_H

#include "sys/types.h"
#include "util/types/list.h"

typedef struct {
	uint32_t start;
	uint32_t end;
	uint32_t size;
	off_t    offset;
	int      fd;
	int      prot;
	int      type;
} mmap_block_t;

typedef struct {
	uint32_t start;
	list_t*  mmap_blocks;
} mmap_info_t;

mmap_block_t*  k_mem_mmap_allocate_block(mmap_info_t* info, void* start, uint32_t size, int prot, int flags);
mmap_block_t*  k_mem_mmap_get_mapping(mmap_info_t* info, uint32_t addr);
void           k_mem_mmap_free_block(mmap_info_t* info, mmap_block_t* block);
void           k_mem_mmap_mark_dirty(mmap_block_t* block);
void           k_mem_mmap_sync_block(mmap_block_t* block, int flags);
void           k_mem_mmap_sync(mmap_info_t* info, int flags);
list_t*        k_mem_mmap_get_mappings_for_fd(int fd, off_t offset);
uint8_t        k_mem_mmap_handle_pagefault(uint32_t address, int code);

#endif
