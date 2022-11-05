#ifndef __K_INT_ISR_H
#define __K_INT_ISR_H

#include <stdint.h>

typedef struct interrupt_context {
    uint32_t edi, esi, ebp, unused, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, esp, ss;
} interrupt_context_t;

typedef interrupt_context_t* (*isr_handler_t)(interrupt_context_t*);

void k_int_isr_init();

void k_int_isr_setup_handler(uint8_t int_no, isr_handler_t handler);

#endif