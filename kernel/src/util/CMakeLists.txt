set(SOURCES 
    libk.c
    asm_wrappers.c
    panic.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})