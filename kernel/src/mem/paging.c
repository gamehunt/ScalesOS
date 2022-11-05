#include "int/isr.h"
#include "mem/heap.h"
#include "mem/pmm.h"
#include "util/panic.h"
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

extern uint32_t k_mem_paging_get_fault_addr();

interrupt_context_t* __pf_handler(interrupt_context_t* ctx){
    char buffer[1024];
    sprintf(buffer, "Page fault at 0x%.8x", k_mem_paging_get_fault_addr());
    k_panic(buffer, ctx);
    __builtin_unreachable();
}

void k_mem_paging_init(){
    k_int_isr_setup_handler(14, __pf_handler);
    uint32_t* pd  = (uint32_t*) (k_mem_paging_get_pd() + VIRTUAL_BASE); // create recursive mapping manually
    uint32_t phys = (uint32_t)&pd[1023] - VIRTUAL_BASE; 
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
        pmm_frame_t frame = k_mem_pmm_alloc_frames(1);
        if(!frame){
            k_panic("Out of memory. Failed to allocate page table.", 0);
        }
        page_directory[pd_index] =  frame | flags | 0x01; //TODO if allocated frame is big, then make a 4MB page
    }

    uint32_t *pt      = ((uint32_t*)0xFFC00000) + (0x400 * pd_index);
    uint32_t pt_index = PTE(vaddr);
    
    if(!paddr){
        paddr = k_mem_pmm_alloc_frames(1);
    }

    pt[pt_index] = ((uint32_t)paddr) | (flags) | 0x01;
}

void  k_mem_paging_map_region(uint32_t vaddr, uint32_t paddr, uint32_t size, uint8_t flags, uint8_t contigous)
{
    if(contigous){
        uint32_t frame = paddr ? paddr : k_mem_pmm_alloc_frames(size);
        for(uint32_t i = 0; i < size; i++){
            k_mem_paging_map(vaddr + 0x1000 * i, frame + 0x1000 * i, flags);
        }
    }else{
        for(uint32_t i = 0; i < size; i++){
            k_mem_paging_map(vaddr + 0x1000 * i, paddr ? paddr + 0x1000 * i : k_mem_pmm_alloc_frames(1), flags);
        }
    }
}

uint32_t k_mem_paging_clone_pd(uint32_t pd){
    uint8_t* src = (uint8_t*) pd;
    uint8_t* copy = k_valloc(0x1000, 0x1000);

    memcpy(copy, src, 0x1000);

    return (uint32_t) copy;
}