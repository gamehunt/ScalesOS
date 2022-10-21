set(SOURCES 
    gdt.asm 
    gdt.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})