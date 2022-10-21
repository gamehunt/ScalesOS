set(SOURCES 
    idt.c
    idt.asm
    isr.c
    isr.asm
    pic.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})