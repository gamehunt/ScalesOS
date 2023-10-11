#include "dev/serial.h"
#include "shared.h"
#include "util/asm_wrappers.h"
#include "util/log.h"
#include <mem/gdt.h>
#include <mem/heap.h>
#include <stdio.h>
#include <string.h>
#include <proc/smp.h>

static struct gdt_entry	    gdt[8];
static struct gdt_ptr		gp;

static tss_entry_t tss;
static tss_entry_t dfs;

static uint32_t df_stack[4096]   __attribute__((aligned(4)));      //Double-fault handler structures
static uint32_t df_cr3[1024]     __attribute__((aligned(0x1000)));

static core initial_core;

void k_mem_gdt_create_entry(struct gdt_entry* gdt_instance, uint8_t idx, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags){
	gdt_instance[idx].base_low  = (base & 0xFFFF);
	gdt_instance[idx].base_mid  = (base >> 16) & 0xFF;
	gdt_instance[idx].base_high = (base >> 24) & 0xFF;
	gdt_instance[idx].limit     = (limit & 0xFFFF);
	gdt_instance[idx].flags     = (limit >> 16) & 0X0F;
	gdt_instance[idx].flags    |= (flags & 0xF0);
	gdt_instance[idx].access    = access;
}

extern void __k_mem_gdt_flush_tss();
extern uint32_t k_mem_paging_get_fault_addr();

static void __k_mem_gdt_df_handler(){
    cli();
    k_dev_serial_write("\r\n", 2);
    char buffer[256];
    sprintf(buffer, "[E] Double fault occured.\r\nEIP: 0x%.8x \r\nEBP: 0x%.8x ESP: 0x%.8x \r\nCR2: 0x%.8x CR3: 0x%.08x\r\n", tss.eip, tss.ebp, tss.esp, k_mem_paging_get_fault_addr(), tss.cr3);
    k_dev_serial_write(buffer, strlen(buffer));
    halt();
}

void k_mem_gdt_init(){
	gp.limit = (sizeof(struct gdt_entry) * 8) - 1;
	gp.base  = (uint32_t)&gdt;

	k_mem_gdt_create_entry(gdt, 0, 0, 0, 0, 0);                               // null      0x0
	k_mem_gdt_create_entry(gdt, 1, 0, 0xFFFFFFFF, 0x9A, 0xCF);                // code      0x8
	k_mem_gdt_create_entry(gdt, 2, 0, 0xFFFFFFFF, 0x92, 0xCF);                // data      0x10
	k_mem_gdt_create_entry(gdt, 3, 0, 0xFFFFFFFF, 0xFA, 0xCF);                // usr code  0x18
	k_mem_gdt_create_entry(gdt, 4, 0, 0xFFFFFFFF, 0xF2, 0xCF);                // usr data  0x20
	k_mem_gdt_create_entry(gdt, 5, (uint32_t) &tss, sizeof(tss), 0xE9, 0x40); // tss       0x28
    k_mem_gdt_create_entry(gdt, 6, (uint32_t) &dfs, sizeof(dfs), 0xE9, 0x40); // dfs       0x30
    k_mem_gdt_create_entry(gdt, 7, (uint32_t) &initial_core, sizeof(initial_core), 0x92, 0x40);                         // core data 0x38

	memset(&tss, 0, sizeof tss);
	tss.ss0  = 0x10;
	tss.esp0 = 0x0;

    memset(df_cr3, 0, 4096);
    df_cr3[0]                   = 0x00000083;
    df_cr3[VIRTUAL_BASE >> 22]  = 0x00000083;

    memset(df_stack, 0, sizeof(df_stack));

    memset(&dfs, 0, sizeof(dfs));
    dfs.prev_tss = 0x28;
    dfs.es       = 0x10;
    dfs.cs       = 0x8;
    dfs.ds       = 0x10;
    dfs.fs       = 0x10;
    dfs.gs       = 0x38;
    dfs.ss       = 0x10;
    dfs.esp      = ((uint32_t) &df_stack[0]) + MB(1);
    dfs.ebp      = dfs.esp;
    dfs.ss0      = 0x10;
    dfs.esp0     = ((uint32_t) &df_stack[0]) + MB(1);
    dfs.eip      = (uint32_t) &__k_mem_gdt_df_handler;
    dfs.cr3      = ((uint32_t) &df_cr3[0]) - VIRTUAL_BASE;

    k_mem_load_gdt((uint32_t)&gp);
    __k_mem_gdt_flush_tss();

    initial_core.gdt = gdt;
    initial_core.tss = &tss;
}

void k_mem_gdt_set_stack(uint32_t stack){
	current_core->tss->esp0 = stack;
}

void k_mem_gdt_set_directory(uint32_t dir){
    current_core->tss->cr3 = dir;
}

void k_mem_gdt_init_core(){
    struct gdt_entry* gdt_copy = (struct gdt_entry*) k_malloc(sizeof(struct gdt_entry) * 8);
    memcpy(gdt_copy, &gdt, sizeof(struct gdt_entry) * 7);
    struct gdt_ptr* gdt_copy_ptr = (struct gdt_ptr*) k_malloc(sizeof(struct gdt_ptr));
    memcpy(gdt_copy_ptr, &gp, sizeof(struct gdt_ptr));

    gdt_copy_ptr->base = (uint32_t) gdt_copy;

    core* core_info = k_malloc(sizeof(core));

    tss_entry_t* tss_copy = (tss_entry_t*) k_malloc(sizeof(tss_entry_t));
    memcpy(tss_copy, &tss, sizeof(tss_entry_t));

	k_mem_gdt_create_entry(gdt_copy, 5, (uint32_t) tss_copy, sizeof(tss_entry_t), 0xE9, 0x40); // tss       0x28
    k_mem_gdt_create_entry(gdt_copy, 7, (uint32_t) core_info, sizeof(core), 0x92, 0x40);
    
    k_mem_load_gdt((uint32_t) gdt_copy_ptr);
    __k_mem_gdt_flush_tss();

    core_info->gdt = gdt_copy;
    core_info->tss = tss_copy;
}
