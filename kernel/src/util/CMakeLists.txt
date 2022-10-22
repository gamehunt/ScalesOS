set(SOURCES 
    libk.c
    asm_wrappers.c
    panic.c
    log.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})