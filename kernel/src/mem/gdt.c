#include "dev/serial.h"
#include "shared.h"
#include "util/asm_wrappers.h"
#include <mem/gdt.h>
#include <stdio.h>
#include <string.h>

static struct gdt_entry	    gdt[7];
static struct gdt_ptr		gp;

static tss_entry_t tss;
static tss_entry_t dfs;

static uint32_t df_stack[1024]  __attribute__((aligned(4)));
static uint32_t df_cr3[1024]    __attribute__((aligned(0x1000)));

void k_mem_gdt_create_entry(uint8_t idx, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags){
	gdt[idx].base_low  = (base & 0xFFFF);
	gdt[idx].base_mid  = (base >> 16) & 0xFF;
	gdt[idx].base_high = (base >> 24) & 0xFF;
	gdt[idx].limit     = (limit & 0xFFFF);
	gdt[idx].flags     = (limit >> 16) & 0X0F;
	gdt[idx].flags    |= (flags & 0xF0);
	gdt[idx].access    = access;
}

extern void __k_mem_gdt_flush_tss();
extern uint32_t k_mem_paging_get_fault_addr();

static void __k_mem_gdt_df_handler(){
    k_dev_serial_write("\r\n", 2);
    char buffer[256];
    sprintf(buffer, "[E] Double fault occured.\r\nEIP: 0x%.8x \r\nEBP: 0x%.8x ESP: 0x%.8x \r\nCR2: 0x%.8x CR3: 0x%.08x\r\n", tss.eip, tss.ebp, tss.esp, k_mem_paging_get_fault_addr(), tss.cr3);
    k_dev_serial_write(buffer, strlen(buffer));
    cli();
    halt();
}

void k_mem_gdt_init(){
	gp.limit = (sizeof(struct gdt_entry) * 7) - 1;
	gp.base  = (uint32_t)&gdt;

	k_mem_gdt_create_entry(0, 0, 0, 0, 0);                               // null     0x0
	k_mem_gdt_create_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);                // code     0x8
	k_mem_gdt_create_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF);                // data     0x10
	k_mem_gdt_create_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);                // usr code 0x18
	k_mem_gdt_create_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);                // usr data 0x20
	k_mem_gdt_create_entry(5, (uint32_t) &tss, sizeof(tss), 0xE9, 0x00); // tss      0x28
    k_mem_gdt_create_entry(6, (uint32_t) &dfs, sizeof(dfs), 0xE9, 0x00); // dfs      0x30

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
    dfs.gs       = 0x10;
    dfs.ss       = 0x10;
    dfs.esp      = ((uint32_t) &df_stack[0]) + MB(1);
    dfs.ebp      = dfs.esp;
    dfs.ss0      = 0x10;
    dfs.esp0     = ((uint32_t) &df_stack[0]) + MB(1);
    dfs.eip      = (uint32_t) &__k_mem_gdt_df_handler;
    dfs.cr3      = ((uint32_t) &df_cr3[0]) - VIRTUAL_BASE;

    k_mem_load_gdt((uint32_t)&gp);
    __k_mem_gdt_flush_tss();
}

void k_mem_gdt_set_stack(uint32_t stack){
	tss.esp0 = stack;
}

void k_mem_gdt_set_directory(uint32_t dir){
    tss.cr3 = dir;
}