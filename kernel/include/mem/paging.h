#ifndef __PAGING_H
#define __PAGING_H

#include <stdint.h>

void      k_mem_paging_init();
void      k_mem_paging_set_pd(uint32_t addr, uint8_t phys);
uint32_t  k_mem_paging_get_pd();
uint32_t  k_mem_paging_virt2phys(uint32_t vaddr);
void      k_mem_paging_map(uint32_t vaddr, uint32_t paddr, uint8_t flags);

#endif