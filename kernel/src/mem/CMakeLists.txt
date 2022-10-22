set(SOURCES 
    gdt.asm 
    gdt.c
    paging.asm
    paging.c
    pmm.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})