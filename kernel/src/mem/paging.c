#include "mem/pmm.h"
#include "util/log.h"
#include <mem/paging.h>

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define PDE(addr) (addr >> 22)
#define PTE(addr) (addr >> 12 & 0x03FF)

#define PD_PRESENT_FLAG (1 << 0)
#define PD_SIZE_FLAG    (1 << 7)

#define PT_PRESENT_FLAG PD_PRESENT_FLAG

uint32_t* page_directory = (uint32_t*) 0xFFFFF000;

extern void* _kernel_end;

uint8_t* heap = 0;

void k_mem_paging_init(){
    heap = (uint8_t*) (&_kernel_end + k_mem_pmm_bitmap_size());
    k_debug("Paging temporary heap at 0x%.8x", heap);
    uint32_t* pd  = (uint32_t*) (k_mem_paging_get_pd() + 0xC0000000); // create first recursive mapping manually
    uint32_t phys = (uint32_t)&pd[1023] - 0xC0000000; 
    pd[1023] = (phys & 0xfffff000) | 3; 
}

uint32_t  k_mem_paging_virt2phys(uint32_t vaddr){
    uint16_t pd_index = PDE(vaddr);
    uint32_t pde = page_directory[pd_index];
    if(!(pde & PD_PRESENT_FLAG)){
        return 0;
    }
    if(pde & PD_SIZE_FLAG){
        return (((pde & 0xffc00000) << 8) | (pde & 0x1fe000)) + ((vaddr % 0x400000) & 0xfffff000);
    }else{
        uint32_t *pt  = ((uint32_t*)0xFFC00000) + (0x400 * pd_index);
        uint32_t  pte = pt[PTE(vaddr)];
        if(pte & PT_PRESENT_FLAG){
            return pte & 0xfffff000; 
        }
        return 0;
    }
}

void k_mem_paging_set_pd(uint32_t addr, uint8_t phys){
    if(!phys){
        addr = k_mem_paging_virt2phys(addr);
    }
    asm volatile("mov %0, %%cr3" :: "r"(addr));
}

void k_mem_paging_map(uint32_t vaddr, uint32_t paddr, uint8_t flags){
    uint32_t pd_index = PDE(vaddr);
    if(!(page_directory[pd_index] & PD_PRESENT_FLAG)){
        memset(heap, 0, 0x1000);
        page_directory[pd_index] = k_mem_paging_virt2phys((uint32_t)heap) | flags | 0x01;
        heap += 0x1000;
    }

    uint32_t *pt      = ((uint32_t*)0xFFC00000) + (0x400 * pd_index);
    uint32_t pt_index = PTE(vaddr);
    
    pt[pt_index] = ((uint32_t)paddr) | (flags) | 0x01;
}