#ifndef __K_MEM_MMAP_H
#define __K_MEM_MMAP_H

#include "sys/types.h"
#include "util/types/list.h"

typedef struct {
	uint32_t start;
	uint32_t size;
	off_t    offset;
	int      fd;
} mmap_blocks_t;

typedef struct {
	uint32_t start;
	list_t*  mmap_blocks;
} mmap_info_t;


mmap_blocks_t* k_mem_mmap_create_block();
void           k_mem_mmap_free_block(mmap_blocks_t* block);

#endif
