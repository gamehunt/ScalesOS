set(SOURCES 
    process.asm
    process.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})