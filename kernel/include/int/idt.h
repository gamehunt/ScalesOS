#ifndef __K_INT_IDT_H
#define __K_INT_IDT_H

#include <stdint.h>

typedef struct __attribute__((packed)) idt_ptr {
    uint16_t limit;
    uint32_t base;
} idt_ptr_t;

typedef struct __attribute__((packed)) idt_entry {
    uint16_t offset_low;
    uint16_t segment;
    uint8_t reserved;
    uint8_t flags;
    uint16_t offset_high;
} idt_entry_t;

void k_int_idt_create_entry(uint8_t idx, uint32_t offset, uint16_t segment,
                            uint8_t dpl, uint8_t type);
void k_int_idt_init();

extern void k_int_load_idt(uint32_t addr);

#endif