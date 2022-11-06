#ifndef __K_MEM_PAGING_H
#define __K_MEM_PAGING_H

#include <stdint.h>

void     k_mem_paging_init();
uint32_t k_mem_paging_get_pd(uint8_t phys);
void     k_mem_paging_set_pd(uint32_t addr, uint8_t phys, uint8_t force);
uint32_t k_mem_paging_clone_pd(uint32_t pd);
uint32_t k_mem_paging_virt2phys(uint32_t vaddr);
void     k_mem_paging_map(uint32_t vaddr, uint32_t paddr, uint8_t flags);
void     k_mem_paging_map_region(uint32_t vaddr, uint32_t paddr, uint32_t size,
                             uint8_t flags, uint8_t cont);

#endif