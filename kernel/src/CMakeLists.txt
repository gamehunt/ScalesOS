set(SOURCES 
    boot.asm 
    kernel.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})

add_subdirectory(dev)
add_subdirectory(util)
add_subdirectory(mem)   
add_subdirectory(int) 