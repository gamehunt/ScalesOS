#ifndef __ISR_H
#define __ISR_H

#include <stdint.h>

typedef struct interrupt_context{
    uint32_t esi, edi, ebp, esp;
    uint32_t ebx, edx, ecx, eax;
    uint8_t  int_no, err_code;
}interrupt_context_t;

typedef void (*isr_handler_t)(interrupt_context_t);

void k_int_isr_init();

void k_int_isr_setup_handler(uint8_t int_no, isr_handler_t handler);

#endif