set(SOURCES 
    vfs.c
    tar.c
    ramdisk.c
    pipe.c
	null.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})
