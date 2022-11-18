#include "kernel.h"
#include "mem/paging.h"
#include "proc/spinlock.h"
#include "util/log.h"
#include "util/panic.h"
#include "string.h"
#include <mem/heap.h>

#define M_BLOCK_FREE    (1 << 0)
#define M_BLOCK_ALIGNED (1 << 1)

//mem_block_t* M_HEADER(void* addr);
#define M_HEADER(addr)     (mem_block_t*)(((uint32_t)addr) - sizeof(mem_block_t))    
//void* M_MEMORY(mem_header_t* addr);     
#define M_MEMORY(addr)     (((uint32_t)addr) + sizeof(mem_block_t))

extern void *_kernel_end;

struct __attribute__((packed)) mem_block{
    struct mem_block* next;
    uint32_t          size;
    uint8_t           flags;
};

typedef struct mem_block mem_block_t;

static mem_block_t* heap      = (mem_block_t*) HEAP_START;

static spinlock_t heap_lock = 0;

static uint8_t __k_mem_heap_is_valid_block(mem_block_t* block){
    uint32_t addr = (uint32_t) block;
    return addr >= HEAP_START && addr < HEAP_END && block->size;
}

static void __k_mem_heap_init_block(mem_block_t* block, uint32_t size){
    if(!size){
        k_panic("Tried to initialize zero-size block. Kmalloc failure.", 0);
        __builtin_unreachable();
    }

    block->next  = 0;
    block->size  = size;
    block->flags = M_BLOCK_FREE;
}

static K_STATUS __k_mem_split_block(mem_block_t* src, mem_block_t** splitted, uint32_t size){
    if(src->size < size){
        return K_STATUS_ERR_GENERIC;
    }

    if(!__k_mem_heap_is_valid_block(src)){
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
    while(__k_mem_heap_is_valid_block(src)){
        if(src->flags & M_BLOCK_FREE && __k_mem_heap_is_valid_block(src->next) && src->next->flags & M_BLOCK_FREE){
            src->size += sizeof(mem_block_t) + src->next->size;
            src->next = src->next->next;
        }
        src = src->next;
    }
}

void __k_d_mem_heap_print_block(mem_block_t* src){
    k_debug("0x%.8x: 0x%.8x %d 0x%.2x", src, src->next, src->size, src->flags);
}

void k_d_mem_heap_print(){
    mem_block_t* src = heap;
    while(__k_mem_heap_is_valid_block(src)){
        __k_d_mem_heap_print_block(src);
        src = src->next;
    }
}

K_STATUS k_mem_heap_init(){
    k_mem_paging_map_region(HEAP_START, 0, (HEAP_SIZE / 0x1000), 0x3, 0);
    __k_mem_heap_init_block(heap, HEAP_SIZE - sizeof(mem_block_t));
    return K_STATUS_OK;
}

void* k_mem_heap_alloc(uint32_t size){
    LOCK(heap_lock)

    __k_mem_merge();

    mem_block_t* block = heap;
    while(__k_mem_heap_is_valid_block(block) && (!(block->flags & M_BLOCK_FREE) 
    || (block->size < size 
    || (block->size > size && block->size < size + sizeof(mem_block_t) + 1)))){
        block = block->next;
    }

    if(!__k_mem_heap_is_valid_block(block)){
        UNLOCK(heap_lock);
        return 0;
    }

    if(block->size == size){
        block->flags &= ~M_BLOCK_FREE;
        UNLOCK(heap_lock)
        return (void*) (M_MEMORY(block));
    }else{
        mem_block_t* sblock;
        if(!IS_OK(__k_mem_split_block(block, &sblock, size))){
            k_panic("Block split failed. Kmalloc failure.", 0);
            __builtin_unreachable();
        }
        block->flags &= ~M_BLOCK_FREE;
        UNLOCK(heap_lock)
        return (void*) (M_MEMORY(block));
    }
}

void k_mem_heap_free(void* ptr){
    LOCK(heap_lock)
    mem_block_t* header = M_HEADER(ptr);
    if(!__k_mem_heap_is_valid_block(header) || header->flags & M_BLOCK_FREE){
        k_warn("Tried to free invalid pointer: 0x%.8x", ptr);
        UNLOCK(heap_lock)
        return;
    }
    header->flags |= M_BLOCK_FREE;
    UNLOCK(heap_lock)
}

void* k_mem_heap_realloc(void* old, uint32_t size){
    mem_block_t* hdr = M_HEADER(old);
    void* new_ptr    = k_malloc(size);
    uint32_t copy_size = 0;
    if(size > hdr->size){
        copy_size = hdr->size;
    }else{
        copy_size = size;
    }
    memmove(new_ptr, old, copy_size);
    k_free(old);
    return new_ptr;
}

void* k_mem_heap_calloc(uint32_t amount, uint32_t size){
    uint8_t* mem = (uint8_t*) k_malloc(amount * size);
    memset(mem, 0, amount * size);
    return (void*) mem;
}

void  k_mem_heap_vfree(void* mem){
    k_free(((void**)mem)[-1]);
}

void* k_mem_heap_valloc(uint32_t size, uint32_t alignment){
	void* p1;  // original block
    void** p2; // aligned block
    int offset = alignment - 1 + sizeof(void*);
    p1 = (void*)k_malloc(size + offset);
    p2 = (void**)(((size_t)(p1) + offset) & ~(alignment - 1));
    p2[-1] = p1;
    return p2;
}