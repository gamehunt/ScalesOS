set(SOURCES 
    tree.c
	list.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})
