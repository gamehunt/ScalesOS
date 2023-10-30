#include "mem/mmio.h"
#include "kernel/mem/paging.h"
#include "mem/memory.h"
#include "mem/paging.h"
#include "types/list.h"

#include <stddef.h>

static list_t* mappings = NULL;

static void* __k_mem_mmio_get_mapping(uint32_t frame) {
	for(uint32_t i = 0; i < mappings->size; i++) {
		if(k_mem_paging_virt2phys((uint32_t) mappings->data[i]) == frame) {
			return mappings->data[i];
		}
	}
	return NULL;
}

uint32_t k_mem_mmio_map_register(uint32_t address) {
	if(!mappings) {
		mappings = list_create();
	} 

	uint32_t page = address / 0x1000;

	void* map = __k_mem_mmio_get_mapping(page);

	if(!map) {
		map = k_map(page, 1, PAGE_PRESENT | PAGE_WRITABLE);
		list_push_back(mappings, map);
	}

	return (uint32_t) map + address % 0x1000;
}
