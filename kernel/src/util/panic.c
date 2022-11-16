#include <stdio.h>
#include <util/asm_wrappers.h>
#include <util/panic.h>
#include "mod/symtable.h"
#include "proc/process.h"

struct stackframe {
  struct stackframe* ebp;
  uint32_t eip;
};

extern void* _kernel_end;

static void __k_panic_stacktrace(uint32_t stack){
    struct stackframe* frame = (struct stackframe*) stack;
    uint8_t depth = 0;
    while(frame && (uint32_t) frame < (uint32_t) &_kernel_end){
        uint32_t eip = frame->eip;
        sym_t* sym = k_mod_symtable_get_nearest_symbol(eip);
        if(sym){
            printf("%d: <%s + 0x%x> \r\n", depth, sym->name, eip - sym->addr);
        }
        frame = frame->ebp;
        depth++;
    }
}

void k_panic(const char* reason, interrupt_context_t* ctx) {
    printf("!!!!!!!!!!!!! Kernel panic !!!!!!!!!!!!!\r\n");
    printf("Reason: %s\r\n", reason);
    process_t* cur_proc = k_proc_current_process();
    if(cur_proc){
        printf("Current process: %s (%d)\r\n", cur_proc->name, cur_proc->pid);
    }
    printf("Dump:\r\n");
    if (ctx) {
        printf("EAX: 0x%.8x EBX: 0x%.8x ECX: 0x%.8x EDX: 0x%.8x\r\n", ctx->eax,
               ctx->ebx, ctx->ecx, ctx->edx);
        printf("EDI: 0x%.8x ESI: 0x%.8x\r\nEBP: 0x%.8x ESP: 0x%.8x\r\n",
               ctx->edi, ctx->esi, ctx->ebp, ctx->esp);
        sym_t* nearest = k_mod_symtable_get_nearest_symbol(ctx->eip);
        if(nearest){
            printf("EIP: 0x%.8x (<%s + 0x%x>) ", ctx->eip, nearest->name, ctx->eip - nearest->addr);
        }else{
            printf("EIP: 0x%.8x (Unknown) ", ctx->eip);
        }
        printf("EFL: 0x%.8x\r\n", ctx->eflags);
        printf("Stacktrace: \r\n");
        __k_panic_stacktrace(ctx->ebp);
    } else {
        printf("Not available.\r\n");
    }
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    cli();
    halt();
}