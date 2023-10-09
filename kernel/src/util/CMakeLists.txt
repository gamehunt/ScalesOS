set(SOURCES 
    libk.c
    asm_wrappers.c
    panic.c
    log.c
    path.c
	exec.c
	perf.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})

add_subdirectory(types)
