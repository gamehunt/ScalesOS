set(SOURCES 
    tree.c
	list.c
	ringbuffer.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})
