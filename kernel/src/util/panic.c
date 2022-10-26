#include <stdio.h>
#include <util/asm_wrappers.h>
#include <util/panic.h>

void k_panic(const char* reason, interrupt_context_t* ctx) {
    printf("!!!!!!!!!!!!! Kernel panic !!!!!!!!!!!!!\r\n");
    printf("Reason: %s\r\n", reason);
    printf("Dump:\r\n");
    if (ctx) {
        printf("EAX: 0x%.8x EBX: 0x%.8x ECX: 0x%.8x EDX: 0x%.8x\r\n", ctx->eax,
               ctx->ebx, ctx->ecx, ctx->edx);
        printf("EDI: 0x%.8x ESI: 0x%.8x\r\nEBP: 0x%.8x ESP: 0x%.8x\r\n",
               ctx->edi, ctx->esi, ctx->ebp, ctx->esp);
    } else {
        printf("Not available.");
    }
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    cli();
    halt();
}