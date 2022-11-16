#include "util/panic.h"
#include <int/isr.h>
#include <int/idt.h>

#include <stdio.h>

#include <util/asm_wrappers.h>
#include <string.h>

#define ISR(x) _isr##x
#define DEFN_ISR(x) extern void ISR(x)(); 

DEFN_ISR(0)
DEFN_ISR(1)
DEFN_ISR(2)
DEFN_ISR(3)
DEFN_ISR(4)
DEFN_ISR(5)
DEFN_ISR(6)
DEFN_ISR(7)
DEFN_ISR(8)
DEFN_ISR(9)
DEFN_ISR(10)
DEFN_ISR(11)
DEFN_ISR(12)
DEFN_ISR(13)
DEFN_ISR(14)
DEFN_ISR(15)
DEFN_ISR(16)
DEFN_ISR(17)
DEFN_ISR(18)
DEFN_ISR(19)
DEFN_ISR(20)
DEFN_ISR(21)
DEFN_ISR(22)
DEFN_ISR(23)
DEFN_ISR(24)
DEFN_ISR(25)
DEFN_ISR(26)
DEFN_ISR(27)
DEFN_ISR(28)
DEFN_ISR(29)
DEFN_ISR(30)
DEFN_ISR(31)

static isr_handler_t isr_handlers[256];

static const char *panic_messages[] = {
	"Division by zero",
	"Debug",
	"Non-maskable interrupt",
	"Breakpoint",
	"Detected overflow",
	"Out-of-bounds",
	"Invalid opcode",
	"No coprocessor",
	"Double fault",
	"Coprocessor segment overrun",
	"Bad TSS", /* 10 */
	"Segment not present",
	"Stack fault",
	"General protection fault",
	"Page fault",
	"Unknown interrupt",
	"Coprocessor fault",
	"Alignment check",
	"Machine check",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved"};

interrupt_context_t* __k_int_isr_dispatcher(interrupt_context_t* ctx){
    if(!isr_handlers[ctx->int_no]){
        char buffer[1024];
        sprintf(buffer, "Unhandled exception: %s (%d). Error code: %d", panic_messages[ctx->int_no], ctx->int_no, ctx->err_code);
        k_panic(buffer, ctx);
        __builtin_unreachable();
    }else{
        return isr_handlers[ctx->int_no](ctx);
    }
}

void k_int_isr_init(){
    memset(isr_handlers, 0, sizeof(isr_handler_t) * 256);

    k_int_idt_create_entry(0,  (uint32_t) &ISR(0),  0x8, 0x0, 0xE);
    k_int_idt_create_entry(1,  (uint32_t) &ISR(1),  0x8, 0x0, 0xE);
    k_int_idt_create_entry(2,  (uint32_t) &ISR(2),  0x8, 0x0, 0xE);
    k_int_idt_create_entry(3,  (uint32_t) &ISR(3),  0x8, 0x0, 0xE);
    k_int_idt_create_entry(4,  (uint32_t) &ISR(4),  0x8, 0x0, 0xE);
    k_int_idt_create_entry(5,  (uint32_t) &ISR(5),  0x8, 0x0, 0xE);
    k_int_idt_create_entry(6,  (uint32_t) &ISR(6),  0x8, 0x0, 0xE);
    k_int_idt_create_entry(7,  (uint32_t) &ISR(7),  0x8, 0x0, 0xE);
    k_int_idt_create_entry(8,  0,  0x30, 0x0, 0x5); // Double faults have their own tss
    k_int_idt_create_entry(9,  (uint32_t) &ISR(9),  0x8, 0x0, 0xE);
    k_int_idt_create_entry(10, (uint32_t) &ISR(10), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(11, (uint32_t) &ISR(11), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(12, (uint32_t) &ISR(12), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(13, (uint32_t) &ISR(13), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(14, (uint32_t) &ISR(14), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(15, (uint32_t) &ISR(15), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(16, (uint32_t) &ISR(16), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(17, (uint32_t) &ISR(17), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(18, (uint32_t) &ISR(18), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(19, (uint32_t) &ISR(19), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(20, (uint32_t) &ISR(20), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(21, (uint32_t) &ISR(21), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(22, (uint32_t) &ISR(22), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(23, (uint32_t) &ISR(23), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(24, (uint32_t) &ISR(24), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(25, (uint32_t) &ISR(25), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(26, (uint32_t) &ISR(26), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(27, (uint32_t) &ISR(27), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(28, (uint32_t) &ISR(28), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(29, (uint32_t) &ISR(29), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(30, (uint32_t) &ISR(30), 0x8, 0x0, 0xE);
    k_int_idt_create_entry(31, (uint32_t) &ISR(31), 0x8, 0x0, 0xE);
}

void k_int_isr_setup_handler(uint8_t int_no, isr_handler_t handler){
    isr_handlers[int_no] = handler;
}