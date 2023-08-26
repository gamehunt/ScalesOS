set(SOURCES 
    vfs.c
    tar.c
    ramdisk.c
    pipe.c
	tmpfs.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})
