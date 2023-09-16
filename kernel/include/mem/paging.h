#ifndef __K_MEM_PAGING_H
#define __K_MEM_PAGING_H

#include "types.h"
#include <stdint.h>

#define  IS_VALID_PTR(x) (k_mem_paging_virt2phys(x) != 0)

union page_directory_entry {
	struct {
		uint32_t present:   1;
		uint32_t writeable: 1;
		uint32_t user:      1;
		uint32_t wt:        1;
		uint32_t nocache:   1;
		uint32_t accessed:  1;
		uint32_t unused1:   1;
		uint32_t size:      1;
		uint32_t unused2:   4;
		uint32_t page:      20;
	} data;
	uint32_t raw;
};

union page_table_entry {
	struct {
		uint32_t present:  1;
		uint32_t writable: 1;
		uint32_t user:     1;
		uint32_t wt:       1;
		uint32_t nocache:  1;
		uint32_t accessed: 1;
		uint32_t dirty:    1;
		uint32_t pat:      1;
		uint32_t global:   1;
		uint32_t unused:   3;
		uint32_t page:     20;
	} data;
	uint32_t raw;
};

#define PAGE_PRESENT  (1 << 0)
#define PAGE_WRITABLE (1 << 1)
#define PAGE_USER     (1 << 2)
#define PAGE_WT       (1 << 3)
#define PAGE_NOCACHE  (1 << 4)
#define PAGE_GLOBAL   (1 << 8)

#define PD_SET_FORCE  (1 << 0)

#define PAGE_ADDRESS(page) (page << 12)

typedef union page_directory_entry pde_t;
typedef union page_table_entry     pte_t;

void     k_mem_paging_init();
pde_t*   k_mem_paging_get_page_directory(paddr_t* phys);
void     k_mem_paging_set_page_directory(pde_t* addr, uint8_t flags);
pde_t*   k_mem_paging_clone_page_directory(pde_t* pd, paddr_t* phys);
pde_t*   k_mem_paging_clone_root_page_directory(paddr_t* phys);
paddr_t  k_mem_paging_virt2phys(vaddr_t vaddr);
void     k_mem_paging_map(vaddr_t vaddr, paddr_t paddr, uint8_t flags);
void     k_mem_paging_unmap(vaddr_t vaddr);
void     k_mem_paging_map_region(vaddr_t vaddr, paddr_t paddr, uint32_t size,
                             uint8_t flags, uint8_t cont);

#endif
