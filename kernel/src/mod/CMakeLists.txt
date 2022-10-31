set(SOURCES 
    modules.c
    elf.c
    symtable.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})