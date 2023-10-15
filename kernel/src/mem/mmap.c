#include "mem/mmap.h"
#include "mem/heap.h"
#include "mem/memory.h"
#include "mem/paging.h"
#include "proc/process.h"
#include "sys/mman.h"
#include "util/fd.h"
#include "util/log.h"
#include "util/types/list.h"

static list_t* __shared_mappings;

mmap_block_t*  k_mem_mmap_allocate_block(mmap_info_t* info, void* start, uint32_t size, int prot, int flags) {
	uint32_t start_addr = 0;
	uint32_t pages = 0;

	if(size % 0x1000) {
		pages = (size / 0x1000) + 1;
	} else {
		pages = size / 0x1000;
	}

	k_debug("mmap: allocating %d pages", pages);

	if(flags == MAP_FIXED) {
		if(!start) {
			return NULL;
		}

		if((uint32_t) start % 0x1000) {
			return NULL;
		}
		
		for(size_t i = 0; i < pages; i++) {
			if((((uint32_t) start + i * 0x1000) >= VIRTUAL_BASE) || k_mem_paging_virt2phys(((uint32_t) start) + i * 0x1000)) {
				return NULL;
			}
		}
		
		start_addr = (uint32_t) start;
	} else {
		if(info->start + pages * 0x1000 >= USER_STACK_END) {
			return NULL;
		}
		start_addr = info->start;
		info->start += pages * 0x1000;
		k_debug("mmap: moved start to 0x%.8x", info->start);
	}

	uint8_t map_flags = PAGE_PRESENT | PAGE_USER;
	if(prot & PROT_WRITE) {
		map_flags |= PAGE_WRITABLE;
	}

	if(flags != MAP_PRIVATE) {
		k_debug("mmap: mapping 0x%.8x-0x%.8x", start_addr, start_addr + pages * 0x1000);
		k_mem_paging_map_region(start_addr, 0, pages, map_flags, 0);
	}

	mmap_block_t* block = k_calloc(1, sizeof(mmap_block_t));
	block->start = (uint32_t) start_addr;
	block->end   = (uint32_t) start_addr + pages * 0x1000;
	block->size  = size;
	block->type  = flags;
	block->prot  = prot;

	list_push_back(info->mmap_blocks, block);

	if(flags == MAP_SHARED) {
		if(!__shared_mappings) {
			__shared_mappings = list_create();
		}
		list_push_back(__shared_mappings, block);
	}

	return block;
}

mmap_block_t* k_mem_mmap_get_mapping(mmap_info_t* info, uint32_t addr) {
	for(size_t i = 0; i < info->mmap_blocks->size; i++) {
		mmap_block_t* block = info->mmap_blocks->data[i];
		if(block->start <= addr && block->end >= addr) {
			return block;
		}
	}
	return NULL;
}

void k_mem_mmap_free_block(mmap_info_t* info, mmap_block_t* block) {
	list_delete_element(info->mmap_blocks, block);
	if(block->type == MAP_SHARED) {
		list_delete_element(__shared_mappings, block);
	}
	k_debug("mmap: unmapping 0x%.8x - 0x%.8x (%d pages)", block->start, block->end, (block->end - block->start) / 0x1000);
	k_mem_paging_unmap_and_free_region(block->start, (block->end - block->start) / 0x1000);
	k_free(block);
}

list_t* k_mem_mmap_get_mappings_for_fd(int fd, off_t offset) {
	list_t* result = list_create();
	for(size_t i = 0; i < __shared_mappings->size; i++) {
		mmap_block_t* block = __shared_mappings->data[i];
		if(block->fd == fd && block->offset <= offset) {
			list_push_back(result, block);
		}
	}
	return result;
}

void k_mem_mmap_mark_dirty(mmap_block_t* block) {
	k_mem_paging_unmap_and_free_region(block->start, (block->end - block->start) / 0x1000);
}

void k_mem_mmap_sync_block(mmap_block_t* block, int flags UNUSED) {
	if(block->type != MAP_SHARED) {
		return;
	}

	fd_list_t* list = &k_proc_process_current()->fds;
	fd_t*      fdt  = fd2fdt(list, block->fd);

	if(!fdt) {
		return;
	}

	k_fs_vfs_write(fdt->node, block->offset, block->size, (void*) block->start);
}

void k_mem_mmap_sync(mmap_info_t* info, int flags) {
	for(size_t i = 0; i < info->mmap_blocks->size; i++) {
		mmap_block_t* block = info->mmap_blocks->data[i];
		k_mem_mmap_sync_block(block, flags);
	}
}

uint8_t k_mem_mmap_handle_pagefault(uint32_t address, int code) {
	process_t* proc = k_proc_process_current();
	mmap_block_t* block = k_mem_mmap_get_mapping(&proc->image.mmap_info, address);

	if(!block) {
		return 1;
	}

	if(block->prot == PROT_NONE) {
		return 2;
	}

	if(!(block->prot & PROT_READ) && !(code & (1 << 1))) {
		return 3;
	}

	if(!(block->prot & PROT_WRITE) && (code & (1 << 1))) {
		return 4;
	}

	if(block->type != MAP_PRIVATE && block->type != MAP_SHARED) {
		return 5;
	}

	uint32_t page = (address / 0x1000) * 0x1000;

	fd_list_t* list = &proc->fds;
	fd_t*      fdt  = fd2fdt(list, block->fd);

	if(!fdt) {
		return 6;
	}

	if(k_mem_paging_virt2phys(page)) {
		return 7;
	}

	int fl = PAGE_PRESENT | PAGE_USER;
	if(block->prot & PROT_WRITE) {
		fl |= PAGE_WRITABLE;
	}
	
	k_mem_paging_map(page, 0, fl);
	
	uint32_t page_offset = (page - block->start) / 0x1000;
	uint32_t offset      = block->offset + page_offset * 0x1000;
	uint32_t length      = 0x1000 - (address - page);

	k_fs_vfs_read(fdt->node, offset, length, (void*) page);

	return 0;
}
