#include "mem/dma.h"
#include "kernel/mem/paging.h"
#include "mem/memory.h"
#include "mem/pmm.h"

#include <string.h>

void* k_mem_dma_alloc(uint32_t pages, uint32_t* phys) {
	uint32_t frames = k_mem_pmm_alloc_frames(pages);
	if(phys) {
		*phys = frames;
	}
	return k_map(frames, pages, PAGE_PRESENT | PAGE_WRITABLE);
}

void k_mem_dma_free(void* ptr, uint32_t pages) {
	uint32_t frame = k_mem_paging_virt2phys((uint32_t) ptr);
	k_unmap(ptr, pages);
	k_mem_pmm_free(frame, pages);
}
