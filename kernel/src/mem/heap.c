#include "kernel.h"
#include "mem/paging.h"
#include "shared.h"
#include "util/log.h"
#include "util/panic.h"
#include <mem/heap.h>

#define M_BLOCK_FREE    (1 << 0)
#define M_BLOCK_ALIGNED (1 << 1)

//mem_block_t* M_HEADER(void* addr);
#define M_HEADER(addr)     (mem_block_t*)(((uint32_t)addr) - sizeof(mem_block_t))    
//void* M_MEMORY(mem_header_t* addr);     
#define M_MEMORY(addr)     (((uint32_t)addr) + sizeof(mem_block_t))

#define M_IS_VALID_BLOCK(block) ((block) && (((uint32_t)(block)) < (HEAP_START + heap_size)))

#define HEAP_START 0xC1000000
#define HEAP_SIZE MB(1)

extern void *_kernel_end;

struct __attribute__((packed)) mem_block{
    struct mem_block* next;
    uint32_t          size;
    uint8_t           flags;
};

typedef struct mem_block mem_block_t;

static mem_block_t* heap      = (mem_block_t*) HEAP_START;
static uint32_t     heap_size = HEAP_SIZE;

static void __k_mem_heap_init_block(mem_block_t* block, uint32_t size){
    block->next  = 0;
    block->size  = size;
    block->flags = M_BLOCK_FREE;
}

static K_STATUS __k_mem_split_block(mem_block_t* src, mem_block_t** splitted, uint32_t size){
    if(src->size < size){
        return K_STATUS_ERR_GENERIC;
    }

    uint32_t diff = src->size - size;

    *splitted  = (mem_block_t*) (M_MEMORY(src) + size);
    __k_mem_heap_init_block(*splitted, diff - sizeof(mem_block_t));

    src->size          = size;
    (*splitted)->next  = src->next;
    src->next          = *splitted;

    return K_STATUS_OK;
}

static void __k_mem_merge(){
    mem_block_t* src = heap;
    while(M_IS_VALID_BLOCK(src)){
        if(src->flags & M_BLOCK_FREE && src->next && src->next->flags & M_BLOCK_FREE){
            src->size += sizeof(mem_block_t) + src->next->size;
            src->next = src->next->next;
        }
        src = src->next;
    }
}

void k_mem_print(){
    mem_block_t* src = heap;
    while(M_IS_VALID_BLOCK(src)){
        k_info("0x%.8x: 0x%.8x %d 0x%.2x", src, src->next, src->size, src->flags);
        src = src->next;
    }
}

K_STATUS k_mem_heap_init(){
    k_mem_paging_map_region(HEAP_START, 0, (HEAP_SIZE / 0x1000), 0x3);
    __k_mem_heap_init_block(heap, HEAP_SIZE - sizeof(mem_block_t));
    return K_STATUS_OK;
}

void* k_mem_heap_alloc(uint32_t size){
    __k_mem_merge();

    mem_block_t* block = heap;
    while(M_IS_VALID_BLOCK(block) && (!(block->flags & M_BLOCK_FREE) 
    || (block->size < size 
    || (block->size > size && block->size < size + sizeof(mem_block_t))))){
        block = block->next;
    }

    if(!M_IS_VALID_BLOCK(block)){
        k_panic("Out of memory. Kmalloc failure.", 0);
    }

    if(block->size == size){
        block->flags &= ~M_BLOCK_FREE;
        return (void*) (M_MEMORY(block));
    }else{
        mem_block_t* sblock;
        if(!IS_OK(__k_mem_split_block(block, &sblock, size))){
            k_panic("Block split failed. Kmalloc failure.", 0);
        }
        block->flags &= ~M_BLOCK_FREE;
        return (void*) (M_MEMORY(block));
    }
}

void k_mem_heap_free(void* ptr){
    mem_block_t* header = M_HEADER(ptr);
    if(!M_IS_VALID_BLOCK(header) || header->flags & M_BLOCK_FREE){
        return;
    }
    header->flags |= M_BLOCK_FREE;
}
