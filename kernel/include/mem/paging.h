#ifndef __K_MEM_PAGING_H
#define __K_MEM_PAGING_H

#include "types.h"
#include <stdint.h>

#define  IS_VALID_PTR(x) (k_mem_paging_virt2phys(x) != 0)

void     k_mem_paging_init();
addr_t   k_mem_paging_get_pd(uint8_t phys);
void     k_mem_paging_set_pd(addr_t addr, uint8_t phys, uint8_t force);
addr_t   k_mem_paging_clone_pd(vaddr_t pd, uint32_t* phys);
paddr_t  k_mem_paging_virt2phys(vaddr_t vaddr);
void     k_mem_paging_map(vaddr_t vaddr, paddr_t paddr, uint8_t flags);
void     k_mem_paging_unmap(vaddr_t vaddr);
void     k_mem_paging_map_region(vaddr_t vaddr, paddr_t paddr, uint32_t size,
                             uint8_t flags, uint8_t cont);
void*    k_mem_paging_map_mmio(paddr_t pstart, uint32_t size);

#endif
