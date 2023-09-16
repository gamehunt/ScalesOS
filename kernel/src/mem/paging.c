#include "int/isr.h"
#include "mem/gdt.h"
#include "mem/heap.h"
#include "mem/pmm.h"
#include "proc/process.h"
#include "types.h"
#include "util/panic.h"
#include <mem/paging.h>
#include <signal.h>
#include <util/log.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <mem/memory.h>

#define PDE(addr)    (addr >> 22)
#define PTE(addr)    (addr >> 12 & 0x03FF)
#define ADDR(pd, pt) ((pd)*0x400000 + (pt)*0x1000)

#define PT_PTR(pd_index) ((pte_t*) ((0xFFC00000) + (0x1000 * pd_index)))

static volatile pde_t* current_page_directory = 0;
pde_t*                 initial_page_directory = 0;

extern void* _kernel_end;

extern uint32_t k_mem_paging_get_fault_addr();
extern void    __k_mem_paging_invlpg(uint32_t addr);

interrupt_context_t* __pf_handler(interrupt_context_t* ctx) {
	process_t* proc          = k_proc_process_current();
	uint32_t   fault_address = k_mem_paging_get_fault_addr(); 
	if((ctx->err_code & 0x4) && proc) {
		if(fault_address == 0xDEADBEEF) {
			k_proc_process_return_from_signal(ctx);
			return ctx;
		} else {
			k_err("Process %s (%d) caused page fault at 0x%x (0x%x). EIP = 0x%.8x ESP = 0x%.8x EBP = 0x%.8x", proc->name, proc->pid, fault_address, ctx->err_code, 
					ctx->eip, ctx->esp, ctx->ebp);
			k_proc_process_send_signal(proc, SIGSEGV);
			return ctx;
		}
	}
    char buffer[128];
    sprintf(buffer, "Page fault at 0x%.8x. Error code: 0x%x Directory: 0x%.8x", fault_address, ctx->err_code, current_page_directory);
    k_panic(buffer, ctx);
    __builtin_unreachable();
}

void k_mem_paging_init() {
    k_int_isr_setup_handler(14, __pf_handler);

    paddr_t phys; 
	k_mem_paging_get_page_directory(&phys);
    
	pde_t* pd = (pde_t*) (phys + VIRTUAL_BASE);
    initial_page_directory = pd;
	current_page_directory = initial_page_directory;

    pd[1023].raw = phys | PAGE_PRESENT | PAGE_WRITABLE;
}

paddr_t k_mem_paging_virt2phys(vaddr_t vaddr) {
    uint16_t pd_index = PDE(vaddr);
    pde_t pde         = current_page_directory[pd_index];
    if (!(pde.data.present)) {
        return 0;
    }
    if (pde.data.size) {
        return (((pde.raw & 0xffc00000) << 8) | (pde.raw & 0x1fe000)) + ((vaddr % 0x400000) & 0xfffff000) + (vaddr % 0x1000);
    } else {
        pte_t* pt = PT_PTR(pd_index);
        pte_t  pte = pt[PTE(vaddr)];
        if (pte.data.present) {
            return PAGE_ADDRESS(pte.data.page) + (vaddr % 0x1000);
        }
        return 0;
    }
}

extern paddr_t __k_mem_paging_get_pd_phys();
extern vaddr_t __k_mem_paging_set_pd(paddr_t phys);

pde_t* k_mem_paging_get_page_directory(paddr_t* phys) {
    if (phys) {
        *phys = __k_mem_paging_get_pd_phys();
    }
    return current_page_directory;
}

void k_mem_paging_set_page_directory(pde_t* addr, uint8_t flags) {
    if (!addr) {
        addr = initial_page_directory;
    }

	if(!IS_VALID_PTR((uint32_t) addr)) {
		k_panic("Tried to set invalid page directory.", NULL);
	}

    if (!(flags & PD_SET_FORCE)) {
        pde_t* current = k_mem_paging_get_page_directory(NULL);
        if (addr == current) {
            return;
        }
    }
    
	pde_t* va = addr;
    addr = (pde_t*) k_mem_paging_virt2phys((addr_t) addr);

	current_page_directory = va;

    k_mem_gdt_set_directory((uint32_t) addr);
    __k_mem_paging_set_pd((addr_t) addr);
}

void k_mem_paging_unmap(vaddr_t vaddr) {
    uint32_t pd_index = PDE(vaddr);
    if (!(current_page_directory[pd_index].data.present)) {
        return;
    }

    pte_t* pt         = PT_PTR(pd_index);
    uint32_t pt_index = PTE(vaddr);

    pt[pt_index].raw  = 0;

    __k_mem_paging_invlpg(vaddr);
}

void k_mem_paging_map(vaddr_t vaddr, paddr_t paddr, uint8_t flags) {
	if(!flags) {
		flags = PAGE_PRESENT | PAGE_WRITABLE;
	}

    uint32_t pd_index = PDE(vaddr);
    if (!current_page_directory[pd_index].data.present) {
        pmm_frame_t frame = k_mem_pmm_alloc_frames(1);
        if (!frame) {
            k_panic("Out of memory. Failed to allocate page table.", 0);
            __builtin_unreachable();
        }
        current_page_directory[pd_index].raw = frame | flags; // TODO if allocated frame is big, then make a 4MB page
        __k_mem_paging_invlpg((uint32_t) PT_PTR(pd_index));
    }

    pte_t*   pt = PT_PTR(pd_index);
    uint32_t pt_index = PTE(vaddr);

    if (!paddr) {
        paddr = k_mem_pmm_alloc_frames(1);
    }

    pt[pt_index].raw = paddr | flags;

    __k_mem_paging_invlpg(vaddr);
}

void k_mem_paging_map_region(vaddr_t vaddr, paddr_t paddr, uint32_t size,
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

pde_t* k_mem_paging_clone_root_page_directory(paddr_t* phys) {
	return k_mem_paging_clone_page_directory(initial_page_directory, phys);
}

pde_t* k_mem_paging_clone_page_directory(pde_t* src, paddr_t* phys) {
    if (!src) {
        src = k_mem_paging_get_page_directory(NULL);
	}

    pde_t*  copy      = k_valloc(0x1000, 0x1000);
	paddr_t copy_phys = k_mem_paging_virt2phys((uint32_t)copy);

    memcpy(copy, src, 0x1000);

    copy[1023].raw = copy_phys | PAGE_PRESENT | PAGE_WRITABLE;

    uint32_t kernel_pd = PDE(VIRTUAL_BASE);
    pde_t*   prev      = k_mem_paging_get_page_directory(NULL);

    k_mem_paging_set_page_directory(src, 0);

    for(uint32_t i = 0; i < kernel_pd; i++){
        if(src[i].data.present){
            uint32_t frame = k_mem_pmm_alloc_frames(1);
            k_mem_paging_map(PT_TMP_MAP, frame, 0);
            copy[i].raw       = frame | (src[i].raw & 0xFFF);
            pte_t* copy_pt    = (pte_t*) PT_TMP_MAP;
            pte_t* src_pt     = PT_PTR(i);
            for(int j = 0; j < 1024; j++){
                if(src_pt[j].data.present){
                    frame = k_mem_pmm_alloc_frames(1);
                    k_mem_paging_map(PG_TMP_MAP, frame, 0);
                    copy_pt[j].raw       = frame | (src_pt[j].raw & 0xFFF);
                    memcpy((void*) PG_TMP_MAP, (void*) ADDR(i, j), 0x1000);
                    k_mem_paging_unmap(PG_TMP_MAP);
                }
            }
            k_mem_paging_unmap(PT_TMP_MAP);
        }
    }

    k_mem_paging_set_page_directory(prev, 0);

    if (phys) {
        *phys = k_mem_paging_virt2phys((uint32_t)copy);
    }

    return copy;
}
