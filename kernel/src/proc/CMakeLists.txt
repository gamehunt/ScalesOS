set(SOURCES 
    process.asm
    process.c
    smp.c
    smp.asm
	mutex.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})
