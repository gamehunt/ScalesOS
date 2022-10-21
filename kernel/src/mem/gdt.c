#include <mem/gdt.h>

struct gdt_entry	gdt[5];
struct gdt_ptr		gp;

void k_mem_gdt_create_entry(uint8_t idx, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags){
	gdt[idx].base_low  = (base & 0xFFFF);
	gdt[idx].base_mid  = (base >> 16) & 0xFF;
	gdt[idx].base_high = (base >> 24) & 0xFF;
	gdt[idx].limit     = (limit & 0xFFFF);
	gdt[idx].flags     = (limit >> 16) & 0X0F;
	gdt[idx].flags    |= (flags & 0xF0);
	gdt[idx].access    = access;
}

void k_mem_gdt_init(){
	gp.limit = (sizeof(struct gdt_entry) * 5) - 1;
	gp.base  = (uint32_t)&gdt;

	k_mem_gdt_create_entry(0, 0, 0, 0, 0);                // null     0x0
	k_mem_gdt_create_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // code     0x8
	k_mem_gdt_create_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // data     0x10
	k_mem_gdt_create_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // usr code 0x18
	k_mem_gdt_create_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // usr data 0x20

    k_mem_load_gdt((uint32_t)&gp);
}