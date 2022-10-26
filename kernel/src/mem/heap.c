#include "kernel.h"
#include "mem/paging.h"
#include "mem/pmm.h"
#include "shared.h"
#include "util/log.h"
#include <mem/heap.h>

#define HEAP_SIZE MB(16)

//TODO

extern void *_kernel_end;

uint32_t heap = 0;
uint32_t heap_size = HEAP_SIZE;

K_STATUS k_mem_heap_init(){
    heap = ALIGN(k_mem_pmm_bitmap_end() + HEAP_SIZE, 0x1000);
    k_mem_paging_map_region(heap, 0, (HEAP_SIZE / 0x1000), 0x3);
    k_info("Kernel heap start: 0x%.8x", heap);

    return K_STATUS_OK;
}

void* k_mem_heap_alloc(uint32_t size){
    heap += size;
    return (void*)(heap - size);
}

void k_mem_heap_free(void* ptr UNUSED){

}