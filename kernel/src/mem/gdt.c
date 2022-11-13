#include <mem/gdt.h>
#include <string.h>

static struct gdt_entry	    gdt[6];
static struct gdt_ptr		gp;
static tss_entry_t tss;

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

void k_mem_gdt_init(){
	gp.limit = (sizeof(struct gdt_entry) * 6) - 1;
	gp.base  = (uint32_t)&gdt;

	k_mem_gdt_create_entry(0, 0, 0, 0, 0);                               // null     0x0
	k_mem_gdt_create_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);                // code     0x8
	k_mem_gdt_create_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF);                // data     0x10
	k_mem_gdt_create_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);                // usr code 0x18
	k_mem_gdt_create_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);                // usr data 0x20
	k_mem_gdt_create_entry(5, (uint32_t) &tss, sizeof(tss), 0xE9, 0x00); // tss      0x28

	memset(&tss, 0, sizeof tss);
	tss.ss0  = 0x10;
	tss.esp0 = 0x0;

    k_mem_load_gdt((uint32_t)&gp);
    __k_mem_gdt_flush_tss();
}



void k_mem_gdt_set_stack(uint32_t stack){
	tss.esp0 = stack;
}