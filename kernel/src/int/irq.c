#include "dev/timer.h"
#include "proc/process.h"
#include <int/pic.h>
#include <int/idt.h>
#include <int/isr.h>
#include <int/irq.h>

#include <string.h>
#include <stdio.h>

#define IRQ(x) _irq##x
#define DEFN_IRQ(x) extern void IRQ(x)();

DEFN_IRQ(0)
DEFN_IRQ(1)
DEFN_IRQ(2)
DEFN_IRQ(3)
DEFN_IRQ(4)
DEFN_IRQ(5)
DEFN_IRQ(6)
DEFN_IRQ(7)
DEFN_IRQ(8)
DEFN_IRQ(9)
DEFN_IRQ(10)
DEFN_IRQ(11)
DEFN_IRQ(12)
DEFN_IRQ(13)
DEFN_IRQ(14)
DEFN_IRQ(15)

irq_handler_t irq_handlers[16];

void k_int_irq_init(){
    memset(irq_handlers, 0, sizeof(irq_handler_t) * 16);

    k_int_idt_create_entry(32, (uint32_t) &IRQ(0),  0x8, 0x0, 0xE);
    k_int_idt_create_entry(33, (uint32_t) &IRQ(1),  0x8, 0x0, 0xE);
    k_int_idt_create_entry(34, (uint32_t) &IRQ(2),  0x8, 0x0, 0xE);
    k_int_idt_create_entry(35, (uint32_t) &IRQ(3),  0x8, 0x0, 0xE);
    k_int_idt_create_entry(36, (uint32_t) &IRQ(4),  0x8, 0x0, 0xE);
    k_int_idt_create_entry(37, (uint32_t) &IRQ(5),  0x8, 0x0, 0xE);
    k_int_idt_create_entry(38, (uint32_t) &IRQ(6),  0x8, 0x0, 0xE);
    k_int_idt_create_entry(39, (uint32_t) &IRQ(7),  0x8, 0x0, 0xE);
    k_int_idt_create_entry(40, (uint32_t) &IRQ(8),  0x8, 0x0, 0xE);
    k_int_idt_create_entry(41, (uint32_t) &IRQ(9),  0x8, 0x0, 0xE);
    k_int_idt_create_entry(42, (uint32_t) &IRQ(10), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(43, (uint32_t) &IRQ(11), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(44, (uint32_t) &IRQ(12), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(45, (uint32_t) &IRQ(13), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(46, (uint32_t) &IRQ(14), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(47, (uint32_t) &IRQ(15), 0x8, 0x0, 0xE);
}

void k_int_irq_setup_handler(uint8_t int_no, irq_handler_t handler){
    irq_handlers[int_no] = handler;
}

interrupt_context_t* __k_int_irq_dispatcher(interrupt_context_t* ctx){
	PRE_INTERRUPT

    uint8_t irq = ctx->int_no;
    if(irq_handlers[irq]){
        ctx = irq_handlers[irq](ctx);
    }else{
        k_int_pic_eoi(irq);
    }

	POST_INTERRUPT

	return ctx;
}
