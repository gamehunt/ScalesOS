#include <stdio.h>
#include <util/asm_wrappers.h>
#include <util/panic.h>
#include "dev/serial.h"
#include "dev/speaker.h"
#include "mem/paging.h"
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
    while(frame && IS_VALID_PTR((uint32_t) frame)){
        uint32_t eip = frame->eip;
        sym_t* sym = k_mod_symtable_get_nearest_symbol(eip);
        if(sym){
            printf("%d: <%s + 0x%x> (in %s) \r\n", depth, sym->name, eip - sym->addr, sym->module);
        }
        frame = frame->ebp;
        depth++;
    }
}

static void __k_panic_pseudo_stacktrace() {
	printf("1: 0x%.8x\r\n", __builtin_return_address(2));
	printf("2: 0x%.8x\r\n", __builtin_return_address(3));
	printf("3: 0x%.8x\r\n", __builtin_return_address(4));
}

void k_panic(const char* reason, interrupt_context_t* ctx){
    cli(); 
	k_dev_speaker_end();
	char buff[1024];
	if(ctx) {
		sprintf(buff, "\r\n%s EIP: 0x%.8x\r\n", reason, ctx->eip);
	} else {
		sprintf(buff, "\r\n%s\r\n", reason);
	}
	k_dev_serial_putstr(buff);
    printf("!!!!!!!!!!!!! Kernel panic !!!!!!!!!!!!!\r\n");
    printf("Reason: %s\r\n", reason);
    printf("Dump:\r\n");
    if (ctx) {
        printf("EAX: 0x%.8x EBX: 0x%.8x ECX: 0x%.8x EDX: 0x%.8x\r\n", ctx->eax,
               ctx->ebx, ctx->ecx, ctx->edx);
        printf("EDI: 0x%.8x ESI: 0x%.8x\r\nEBP: 0x%.8x ESP: 0x%.8x\r\n",
               ctx->edi, ctx->esi, ctx->ebp, ctx->esp);
        sym_t* nearest = k_mod_symtable_get_nearest_symbol(ctx->eip);
        if(nearest){
            printf("EIP: 0x%.8x (<%s + 0x%x> in %s) ", ctx->eip, nearest->name, ctx->eip - nearest->addr, nearest->module);
        }else{
            printf("EIP: 0x%.8x (Unknown) ", ctx->eip);
        }
        printf("EFL: 0x%.8x\r\n", ctx->eflags);
        printf("Stacktrace: \r\n");
        __k_panic_stacktrace(ctx->ebp);
    } else {
        printf("Not available.\r\n");
        printf("Stacktrace: \r\n");
		__k_panic_pseudo_stacktrace();
    }
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    halt();
}
