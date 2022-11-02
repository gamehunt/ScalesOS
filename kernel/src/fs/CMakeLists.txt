set(SOURCES 
    vfs.c
    tar.c
    ramdisk.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})