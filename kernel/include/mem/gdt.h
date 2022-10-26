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

void k_mem_gdt_create_entry(uint8_t idx, uint32_t base, uint32_t limit,
                            uint8_t access, uint8_t flags);
void k_mem_gdt_init();

extern void k_mem_load_gdt(uint32_t addr);

#endif