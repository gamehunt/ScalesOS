set(SOURCES 
    gdt.asm 
    gdt.c
    paging.asm
    paging.c
    pmm.c
    heap.c
	mmio.c
	mmap.c
	memory.c
	dma.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})
