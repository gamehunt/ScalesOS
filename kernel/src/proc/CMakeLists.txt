set(SOURCES 
    process.asm
    process.c
    smp.c
    smp.asm
)

target_sources(kernel.bin PRIVATE ${SOURCES})
