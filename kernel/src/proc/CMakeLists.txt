set(SOURCES 
    process.asm
    process.c
    smp.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})