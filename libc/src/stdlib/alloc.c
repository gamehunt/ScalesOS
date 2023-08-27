#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "mem/memory.h"
#include "mem/heap.h"

#define M_BLOCK_FREE    (1 << 0)
#define M_BLOCK_ALIGNED (1 << 1)

//mem_block_t* M_HEADER(void* addr);
#define M_HEADER(addr)     (mem_block_t*)(((uint32_t)addr) - sizeof(mem_block_t))    
//void* M_MEMORY(mem_header_t* addr);     
#define M_MEMORY(addr)     (((uint32_t)addr) + sizeof(mem_block_t))

#ifdef __LIBK

#include "util/log.h"
#include "util/panic.h"

mem_block_t* heap      = (mem_block_t*) HEAP_START;

#else

#include "sys/syscall.h"

mem_block_t* heap      = (mem_block_t*) USER_HEAP_START;
static uint32_t heap_size = USER_HEAP_INITIAL_SIZE;

uint32_t __mem_grow_heap(int32_t size) {
	return __sys_grow(size);
}

void    __mem_init_heap() {
	__mem_heap_init_block(heap, heap_size);
}
#endif


static uint32_t __mem_heap_size() {
#ifdef __LIBK
	return HEAP_SIZE;
#else
	return heap_size;
#endif
}

static uint32_t __mem_heap_start() {
#ifdef __LIBK
	return HEAP_START;
#else
	return USER_HEAP_START;
#endif
}

static uint32_t __mem_heap_end() {
#ifdef __LIBK
	return HEAP_END;
#else
	return ((uint32_t)heap) + __mem_heap_size();
#endif
}

uint8_t __mem_heap_is_valid_block(mem_block_t* block){
    uint32_t addr = (uint32_t) block;
    return addr >= __mem_heap_start() && addr < __mem_heap_end() && block->size;
}

void __mem_heap_init_block(mem_block_t* block, uint32_t size){
    if(!size){
#ifdef __LIBK
        k_panic("Tried to initialize zero-size block. Kmalloc failure.", 0);
#else
		abort();
#endif
        __builtin_unreachable();
    }

    block->next  = 0;
    block->size  = size;
    block->flags = M_BLOCK_FREE;
}

static uint8_t __mem_split_block(mem_block_t* src, mem_block_t** splitted, uint32_t size){
    if(src->size < size){
        return 1;
    }

    if(!__mem_heap_is_valid_block(src)){
        return 1;
    }

    uint32_t diff = src->size - size;

    *splitted  = (mem_block_t*) (M_MEMORY(src) + size);
    __mem_heap_init_block(*splitted, diff - sizeof(mem_block_t));

    src->size          = size;
    (*splitted)->next  = src->next;
    src->next          = *splitted;

    return 0;
}

static void __mem_merge(){
    mem_block_t* src = heap;
    while(__mem_heap_is_valid_block(src)){
        if(src->flags & M_BLOCK_FREE && __mem_heap_is_valid_block(src->next) && src->next->flags & M_BLOCK_FREE){
            src->size += sizeof(mem_block_t) + src->next->size;
            src->next = src->next->next;
        }
        src = src->next;
    }
}

static uint32_t __allocated = 0;
static uint32_t __freed     = 0;

void* malloc(size_t size){

    __mem_merge();

    mem_block_t* block            = heap;
	mem_block_t* last_valid_block = heap;
    while(__mem_heap_is_valid_block(block) && (!(block->flags & M_BLOCK_FREE) 
    || (block->size < size 
    || (block->size > size && block->size < size + sizeof(mem_block_t) + 1)))){
		last_valid_block = block;
        block = block->next;
    }

    if(!__mem_heap_is_valid_block(block)){
#ifdef __LIBK
		k_err("Allocation size: %d (used: %d/%d KB)", size, (__allocated - __freed) / 1024, HEAP_SIZE / 1024);
		k_panic("Out of memory.", 0);
#else 
		uint32_t grow = (size + sizeof(mem_block_t) + 1) / 0x1000 + 1;
		block = (mem_block_t*) __mem_grow_heap(grow);
		__mem_heap_init_block(block, grow * 0x1000);
		last_valid_block->next = block;
		heap_size += grow;
#endif
    }

    if(block->size == size){
        block->flags &= ~M_BLOCK_FREE;
        return (void*) (M_MEMORY(block));
    }else{
        mem_block_t* sblock;
        if(__mem_split_block(block, &sblock, size) != 0){
#ifdef __LIBK
            k_panic("Block split failed. Kmalloc failure.", 0);
#else
			abort();
#endif
            __builtin_unreachable();
        }
        block->flags &= ~M_BLOCK_FREE;
		__allocated += size;
        return (void*) (M_MEMORY(block));
    }
}

void *calloc(size_t num, size_t size){
    void* mem = malloc(num * size);
    memset(mem, 0, num * size);
    return mem;
}

void* realloc(void* old, size_t size){
    mem_block_t* hdr = M_HEADER(old);
    void* new_ptr    = malloc(size);
    size_t copy_size = 0;
    if(size > hdr->size){
        copy_size = hdr->size;
    }else{
        copy_size = size;
    }
    memmove(new_ptr, old, copy_size);
    free(old);
    return new_ptr;
}

void free(void* ptr){
    mem_block_t* header = M_HEADER(ptr);
    if(!__mem_heap_is_valid_block(header) || header->flags & M_BLOCK_FREE){
#ifdef __LIBK
        k_warn("Tried to free invalid pointer: 0x%.8x", ptr);
#endif
        return;
    }
    header->flags |= M_BLOCK_FREE;
	__freed += header->size;
}

void vfree(void* mem){
    free(((void**)mem)[-1]);
}

void* valloc(size_t size, size_t alignment){
	void* p1;  // original block
    void** p2; // aligned block
    int offset = alignment - 1 + sizeof(void*);
    p1 = (void*) malloc(size + offset);
    p2 = (void**)(((size_t)(p1) + offset) & ~(alignment - 1));
    p2[-1] = p1;
    return p2;
}
