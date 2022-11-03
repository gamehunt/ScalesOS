#ifndef __K_MEM_GDT_H
#define __K_MEM_GDT_H

#include <stdint.h>

#define GDT_ACCESS_ACCESSED (1 << 0)
#define GDT_ACCESS_RW (1 << 1)
#define GDT_ACCESS_DIRECTION (1 << 2)
#define GDT_ACCESS_EXEC (1 << 3)
#define GDT_ACCESS_TYPE (1 << 4)
#define GDT_ACCESS_PRIV (1 << 5 | 1 << 6)
#define GDT_ACCESS_PRESENT (1 << 7)

#define GDT_FLAG_LONG (1 << 1)
#define GDT_FLAG_SIZE (1 << 2)
#define GDT_FLAG_GRAN (1 << 3)

typedef struct __attribute__((packed)) gdt_ptr {
    uint16_t limit;
    uint32_t base;
} gdt_ptr_t;

typedef struct __attribute__((packed)) gdt_entry {
    uint16_t limit;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t flags;
    uint8_t base_high;
} gdt_entry_t;

typedef struct __attribute__((packed)) tss_entry {
	uint32_t prev_tss; // The previous TSS - with hardware task switching these form a kind of backward linked list.
	uint32_t esp0;     // The stack pointer to load when changing to kernel mode.
	uint32_t ss0;      // The stack segment to load when changing to kernel mode.
	// Everything below here is unused.
	uint32_t esp1; // esp and ss 1 and 2 would be used when switching to rings 1 or 2.
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	uint32_t ldt;
	uint16_t trap;
	uint16_t iomap_base;
} tss_entry_t;

void k_mem_gdt_create_entry(uint8_t idx, uint32_t base, uint32_t limit,
                            uint8_t access, uint8_t flags);
void k_mem_gdt_init();

extern void k_mem_load_gdt(uint32_t addr);

void     k_mem_gdt_set_stack(uint32_t stack);

#endif