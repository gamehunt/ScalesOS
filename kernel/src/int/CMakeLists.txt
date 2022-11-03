set(SOURCES 
    idt.c
    idt.asm
    isr.c
    isr.asm
    irq.c
    irq.asm
    pic.c
    syscall.asm
    syscall.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})