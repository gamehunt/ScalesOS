#include "int/isr.h"
#include "mem/heap.h"
#include "mem/pmm.h"
#include "proc/process.h"
#include "util/panic.h"
#include <mem/paging.h>
#include <util/log.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <mem/memory.h>

#define PDE(addr)    (addr >> 22)
#define PTE(addr)    (addr >> 12 & 0x03FF)
#define ADDR(pd, pt) ((pd)*0x400000 + (pt)*0x1000)

#define PT_PTR(pd_index) ((uint32_t*)((0xFFC00000) + (0x1000 * pd_index)))

#define GET_ADDR(pde)  (pde & 0xFFFFF000)
#define GET_FLAGS(pde) (pde & 0xFFF)

#define PD_PRESENT_FLAG (1 << 0)
#define PD_SIZE_FLAG    (1 << 7)

#define PT_PRESENT_FLAG PD_PRESENT_FLAG

static volatile uint32_t* page_directory = (uint32_t*)0xFFFFF000;
uint32_t __k_mem_paging_initial_directory = 0;

extern void* _kernel_end;

extern uint32_t k_mem_paging_get_fault_addr();
extern void __k_mem_paging_invlpg(uint32_t addr);

interrupt_context_t* __pf_handler(interrupt_context_t* ctx) {
	process_t* proc = k_proc_process_current();
	if((ctx->err_code & 0x4) && proc) {
		k_err("Process %s (%d) caused page fault at 0x%x (0x%x).", proc->name, proc->pid, k_mem_paging_get_fault_addr(), ctx->err_code);
		k_proc_process_exit(proc, 3);
		__builtin_unreachable();
	}
    char buffer[128];
    sprintf(buffer, "Page fault at 0x%.8x. Error code: 0x%x",
            k_mem_paging_get_fault_addr(), ctx->err_code);
    k_panic(buffer, ctx);
    __builtin_unreachable();
}

void k_mem_paging_init() {
    k_int_isr_setup_handler(14, __pf_handler);
    uint32_t phys = k_mem_paging_get_pd(1);
    __k_mem_paging_initial_directory = phys;
    uint32_t* pd = (uint32_t*)(phys + VIRTUAL_BASE);
    pd[1023] = (phys) | 0x03;
}

uint32_t k_mem_paging_virt2phys(uint32_t vaddr) {
    uint16_t pd_index = PDE(vaddr);
    uint32_t pde = page_directory[pd_index];
    if (!(pde & PD_PRESENT_FLAG)) {
        return 0;
    }
    if (pde & PD_SIZE_FLAG) {
        return (((pde & 0xffc00000) << 8) | (pde & 0x1fe000)) +
               ((vaddr % 0x400000) & 0xfffff000);
    } else {
        uint32_t* pt = PT_PTR(pd_index);
        uint32_t pte = pt[PTE(vaddr)];
        if (pte & PT_PRESENT_FLAG) {
            return GET_ADDR(pte);
        }
        return 0;
    }
}

extern uint32_t __k_mem_paging_get_pd_phys();
extern uint32_t __k_mem_paging_set_pd(uint32_t phys);

uint32_t k_mem_paging_get_pd(uint8_t p) {
    if (p) {
        return __k_mem_paging_get_pd_phys();
    }
    return (uint32_t)page_directory;
}

void k_mem_paging_set_pd(uint32_t addr, uint8_t phys, uint8_t force) {
    if (!addr) {
        phys = 1;
        addr = __k_mem_paging_initial_directory;
    }
    if (!force) {
        uint32_t current = k_mem_paging_get_pd(phys);
        if (addr == current) {
            return;
        }
    }
    if (!phys) {
        addr = k_mem_paging_virt2phys(addr);
    }
    __k_mem_paging_set_pd(addr);
}

void k_mem_paging_unmap(uint32_t vaddr) {
    uint32_t pd_index = PDE(vaddr);
    if (!(page_directory[pd_index] & PD_PRESENT_FLAG)) {
        return;
    }

    uint32_t* pt = PT_PTR(pd_index);
    uint32_t pt_index = PTE(vaddr);

    pt[pt_index] = 0;

    __k_mem_paging_invlpg(vaddr);
}

void k_mem_paging_map(uint32_t vaddr, uint32_t paddr, uint8_t flags) {
    uint32_t pd_index = PDE(vaddr);
    if (!(page_directory[pd_index] & PD_PRESENT_FLAG)) {
        pmm_frame_t frame = k_mem_pmm_alloc_frames(1);
        if (!frame) {
            k_panic("Out of memory. Failed to allocate page table.", 0);
            __builtin_unreachable();
        }
        page_directory[pd_index] =
            frame | flags |
            0x03; // TODO if allocated frame is big, then make a 4MB page
        __k_mem_paging_invlpg((uint32_t) PT_PTR(pd_index));
    }

    uint32_t* pt = PT_PTR(pd_index);
    uint32_t pt_index = PTE(vaddr);

    if (!paddr) {
        paddr = k_mem_pmm_alloc_frames(1);
    }

    pt[pt_index] = paddr | flags | 0x03;

    __k_mem_paging_invlpg(vaddr);
}

void k_mem_paging_map_region(uint32_t vaddr, uint32_t paddr, uint32_t size,
                             uint8_t flags, uint8_t contigous) {
    if (contigous) {
        uint32_t frame = paddr ? paddr : k_mem_pmm_alloc_frames(size);
        for (uint32_t i = 0; i < size; i++) {
            k_mem_paging_map(vaddr + 0x1000 * i, frame + 0x1000 * i, flags);
        }
    } else {
        for (uint32_t i = 0; i < size; i++) {
            k_mem_paging_map(
                vaddr + 0x1000 * i,
                paddr ? paddr + 0x1000 * i : k_mem_pmm_alloc_frames(1), flags);
        }
    }
}

uint32_t k_mem_paging_clone_pd(uint32_t pd, uint32_t* phys) {
    if (!pd) {
        pd = k_mem_paging_get_pd(0);
    }

    uint32_t* src = (uint32_t*)pd;
    uint32_t  src_phys = k_mem_paging_virt2phys(pd);

    uint32_t* copy = k_valloc(0x1000, 0x1000);

    memcpy(copy, src, 0x1000);

    copy[1023] = (k_mem_paging_virt2phys((uint32_t)copy)) | 0x03;

    uint32_t kernel_pd = PDE(VIRTUAL_BASE);
    uint32_t prev = k_mem_paging_get_pd(1);
    k_mem_paging_set_pd(src_phys, 1, 0);

    for(uint32_t i = 0; i < kernel_pd; i++){
        if(src[i] & PD_PRESENT_FLAG){
            uint32_t frame = k_mem_pmm_alloc_frames(1);
            k_mem_paging_map(PT_TMP_MAP, frame, 0x3);
            copy[i] = frame | GET_FLAGS(src[i]);
            uint32_t* copy_pt = (uint32_t*) PT_TMP_MAP;
            uint32_t* src_pt  = PT_PTR(i);
            for(int j = 0; j < 1024; j++){
                if(src_pt[j] & PD_PRESENT_FLAG){
                    frame = k_mem_pmm_alloc_frames(1);
                    k_mem_paging_map(PG_TMP_MAP, frame, 0x3);
                    copy_pt[j] = frame | GET_FLAGS(src_pt[j]);
                    memcpy((void*) PG_TMP_MAP, (void*) ADDR(i, j), 0x1000);
                    k_mem_paging_unmap(PG_TMP_MAP);
                }
            }
            k_mem_paging_unmap(PT_TMP_MAP);
        }
    }
    k_mem_paging_set_pd(prev, 1, 0);

    if (phys) {
        *phys = k_mem_paging_virt2phys((uint32_t)copy);
    }

    return (uint32_t)copy;
}

static uint32_t mmio_base = MMIO_START;

void* k_mem_paging_map_mmio(uint32_t pstart, uint32_t size){
    void* ptr = (void*) mmio_base;
    k_mem_paging_map_region((uint32_t) ptr, pstart, size, 0x3, 1);
    mmio_base += size * 0x1000;
    return ptr;
}
