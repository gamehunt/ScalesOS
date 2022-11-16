#include <int/syscall.h>
#include <stdio.h>
#include "int/idt.h"
#include "int/isr.h"
#include "kernel.h"
#include "shared.h"
#include "util/log.h"

extern void _syscall_stub();

interrupt_context_t* __k_int_syscall_dispatcher(interrupt_context_t* ctx UNUSED){
    k_debug("%d %d %d %d %d", ctx->ebx, ctx->ecx, ctx->edx, ctx->edi, ctx->esi);
    ctx->eax = 0;
    return ctx;
}

K_STATUS k_int_syscall_init(){
    k_int_idt_create_entry(0x80, (uint32_t) &_syscall_stub, 0x8, 0x3, 0xE);
    return K_STATUS_OK;
}