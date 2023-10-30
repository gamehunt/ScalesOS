set(SOURCES 
	ringbuffer.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})
