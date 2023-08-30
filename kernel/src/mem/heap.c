#include "kernel.h"
#include "mem/paging.h"
#include "util/log.h"
#include "string.h"
#include "mem/memory.h"
#include <mem/heap.h>

extern uint8_t __mem_heap_is_valid_block(mem_block_t* block);
extern void __mem_heap_init_block(mem_block_t* block, uint32_t size);
extern mem_block_t* heap;

void __k_d_mem_heap_print_block(mem_block_t* src){
    k_debug("0x%.8x: 0x%.8x %d 0x%.2x", src, src->next, src->size, src->flags);
}

void k_d_mem_heap_print(){
    mem_block_t* src = heap;
    while(__mem_heap_is_valid_block(src)){
        __k_d_mem_heap_print_block(src);
        src = src->next;
    }
	k_info("Last next: 0x%.8x", (uint32_t) src);
}

K_STATUS k_mem_heap_init(){
    k_mem_paging_map_region(HEAP_START, 0, (HEAP_SIZE / 0x1000), 0x3, 0);
    __mem_heap_init_block(heap, HEAP_SIZE - sizeof(mem_block_t));
    return K_STATUS_OK;
}
