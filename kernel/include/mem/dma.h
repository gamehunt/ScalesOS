#ifndef __K_MEM_DMA_H
#define __K_MEM_DMA_H

#include <stdint.h>

void* k_mem_dma_alloc(uint32_t pages);
void  k_mem_dma_free(void* ptr, uint32_t pages);

#endif
