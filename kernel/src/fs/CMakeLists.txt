set(SOURCES 
    vfs.c
    ramdisk.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})