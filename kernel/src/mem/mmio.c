#include "mem/mmio.h"
#include "mem/memory.h"
#include "mem/paging.h"
#include "util/log.h"

static uint32_t mmio_pool = MMIO_START;

void  k_mem_mmio_init() {
	k_mem_paging_map_region(MMIO_START, 0, MMIO_SIZE / 0x1000, 0x3, 0x1);
}

void* k_mem_mmio_alloc(enum mmio_size size, paddr_t* phys) {
	uint32_t result = mmio_pool;
	if(phys) {
		*phys = k_mem_paging_virt2phys(result);
	}
	mmio_pool += size * 0x1000;
	return (void*) result;
}

void  k_mem_mmio_free(void* ptr UNUSED) {
	k_warn("FIXME: k_mem_mmio_free() unimplemented.");
}


uint32_t k_mem_mmio_map_register(uint32_t address, uint8_t size) {
	//TODO
	return 0;
}
