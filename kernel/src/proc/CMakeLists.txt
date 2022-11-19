set(SOURCES 
    process.asm
    process.c
    smp.asm
    smp.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})