#include "mem/mmio.h"
#include "kernel/mem/paging.h"
#include "mem/memory.h"
#include "mem/paging.h"

uint32_t k_mem_mmio_map_register(uint32_t address) {
	uint32_t page = address / 0x1000;
	uint32_t register_base = MMIO_START + page * 0x1000;

	if(register_base > MMIO_END) {
		return 0;
	}

	if(!IS_VALID_PTR(register_base)) {
		k_mem_paging_map(register_base, address, PAGE_PRESENT | PAGE_WRITABLE);
	}

	return register_base + address % 0x1000;
}
