set(SOURCES 
    modules.c
    elf.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})