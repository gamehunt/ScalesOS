#include "mem/memory.h"
#include "dev/serial.h"
#include "kernel/mem/paging.h"
#include "mem/paging.h"
#include "proc/spinlock.h"
#include "types.h"
#include "util/log.h"
#include "util/panic.h"

#include <stddef.h>

static vaddr_t     first_free_address = LOWMEM_START;
static spinlock_t  lock = 0;

void* k_map(uint32_t frame, uint32_t size, uint8_t flags) {
	LOCK(lock)
	
	uint32_t start_addr = first_free_address;
	uint32_t offset     = 0;

	while(1) {
		uint32_t i = 0;
		uint8_t  f = 0;
		for(i = 0; i < size; i++) {
			if(k_mem_paging_virt2phys(start_addr + (i + offset) * 0x1000)) {
				f = 1;
				break;
			}
		}

		if(f) {
			while(k_mem_paging_virt2phys(start_addr + (i + offset) * 0x1000)) {
				i++;
			}
			offset = i;
		} else {
			break;
		}

		if(start_addr + offset * 0x1000 >= LOWMEM_END) {
			break;
		}
	}

	if(start_addr + (offset + size) * 0x1000 > LOWMEM_END) {
		k_panic("k_map(): failed to find free address.", NULL);
	}
	
	start_addr += 0x1000 * offset;

	first_free_address  = start_addr + size * 0x1000;

	k_mem_paging_map_region(start_addr, frame, size, flags, 1);

	UNLOCK(lock);

	return (void*) start_addr;
}

void  k_unmap(void* addr, uint32_t size) {
	if((uint32_t) addr >= LOWMEM_START && (uint32_t) addr + size * 0x1000 <= LOWMEM_END) {
		LOCK(lock)
		k_mem_paging_unmap_region((uint32_t) addr, size);
		if((uint32_t) addr < first_free_address) {
			first_free_address = (uint32_t) addr;
		}
		UNLOCK(lock)
	}
}
