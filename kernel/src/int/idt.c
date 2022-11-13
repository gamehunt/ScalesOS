#include "util/log.h"
#include <int/idt.h>

#include <util/asm_wrappers.h>

#include <stdio.h>
#include <string.h>

struct idt_entry idt[256];
struct idt_ptr   idt_ptr;

void k_int_idt_create_entry(uint8_t idx, uint32_t offset, uint16_t segment, uint8_t dpl, uint8_t type){
    idt[idx].offset_low  = offset & 0xffff;
    idt[idx].offset_high = offset >> 16;
    idt[idx].segment     = segment;
    idt[idx].reserved    = 0;
    idt[idx].flags       = ((0xF & type)) | ((dpl & 0x3) << 5) | (1 << 7);
}

void k_int_idt_init(){
    idt_ptr.base  = (uint32_t)&idt;
    idt_ptr.limit = sizeof(struct idt_entry) * 256 - 1;

    memset(&idt, 0, sizeof(struct idt_entry) * 256);

    k_int_load_idt((uint32_t)&idt_ptr);
    sti();
}