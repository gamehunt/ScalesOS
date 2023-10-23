set(SOURCES 
    vfs.c
    tar.c
    ramdisk.c
    pipe.c
	tmpfs.c
	procfs.c
	socket.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})
