#ifndef __K_MEM_MMIO_H
#define __K_MEM_MMIO_H

#include "types.h"

enum mmio_size {
	kb4 = 1,
	kb8,
	kb16,
	kb32,
	kb64
};

#define MMIO_4KB  kb4
#define MMIO_8KB  kb8
#define MMIO_16KB kb16
#define MMIO_32KB kb32
#define MMIO_64KB kb64

void  	 k_mem_mmio_init();
void* 	 k_mem_mmio_alloc(enum mmio_size size, paddr_t* phys);
void  	 k_mem_mmio_free(void* ptr);
uint32_t k_mem_mmio_map_register(uint32_t address, uint8_t size);

#endif
