set(SOURCES 
    boot.asm 
    kernel.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})

add_subdirectory(dev)
add_subdirectory(util)
add_subdirectory(mem)   
add_subdirectory(int) 
add_subdirectory(proc)
add_subdirectory(fs)
add_subdirectory(mod)
add_subdirectory(video)
