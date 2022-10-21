#ifndef __IRQ_H
#define __IRQ_H

#include <int/isr.h>

typedef isr_handler_t irq_handler_t;

void k_int_irq_init();
void k_int_irq_setup_handler(uint8_t int_no, irq_handler_t handler);

#endif