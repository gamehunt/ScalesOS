#include <int/syscall.h>
#include "int/idt.h"
#include "int/isr.h"
#include "kernel.h"
#include "shared.h"
#include "util/log.h"

extern void _syscall_stub();

void __k_int_syscall_dispatcher(interrupt_context_t ctx UNUSED){
    k_info("SYSCALL");
}

K_STATUS k_int_syscall_init(){
    k_int_idt_create_entry(0x80, (uint32_t) &_syscall_stub, 0x8, 0x3, 0xE);
    return K_STATUS_OK;
}

void a(){
    asm volatile("int $0x80");
    while(1);
}