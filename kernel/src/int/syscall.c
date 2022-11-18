#include <int/syscall.h>
#include <stdio.h>
#include "int/idt.h"
#include "int/isr.h"
#include "kernel.h"

#include "util/log.h"
#include <proc/process.h>
#include <string.h>

extern void _syscall_stub();

interrupt_context_t* __k_int_syscall_dispatcher(interrupt_context_t* ctx){
    process_t* cur = k_proc_process_current();
    memcpy((void*) &cur->syscall_state, ctx, sizeof(interrupt_context_t));
    if(ctx->eax == 1){
        ctx->eax = k_proc_process_fork();
    }else{
        k_info("%s", (char*) ctx->ebx);
        ctx->eax = 0;
    }
    return ctx;
}

K_STATUS k_int_syscall_init(){
    k_int_idt_create_entry(0x80, (uint32_t) &_syscall_stub, 0x8, 0x3, 0xE);
    return K_STATUS_OK;
}