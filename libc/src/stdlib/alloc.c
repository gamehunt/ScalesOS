#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "kernel/mem/paging.h"
#include "mem/heap.h"
#include "mem/memory.h"
#include "proc/process.h"
#include "sys/heap.h"
#include "proc/spinlock.h"
#include "util/asm_wrappers.h"

static spinlock_t heap_lock = 0; //TODO
								 
#undef LOCK
#undef UNLOCK

#define LOCK(x)
#define UNLOCK(x)

#ifdef __LIBK

#include "util/log.h"
#include "util/panic.h"

mem_block_t* heap            = (mem_block_t*) HEAP_START;

#else

#include "sys/syscall.h"

mem_block_t* heap         = NULL;
static uint32_t heap_size = USER_HEAP_INITIAL_SIZE;

static uint32_t __mem_grow_heap(int32_t size) {
	return __sys_grow(size);
}

void __mem_init_heap() {
	heap = __mem_grow_heap(0);
	__mem_heap_init_block(heap, heap_size - sizeof(mem_block_t));
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
	return (uint32_t) heap;
}

static uint32_t __mem_heap_end() {
#ifndef __LIBK
	return USER_STACK_END;
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
		fprintf(stderr, "__mem_heap_init_block(): tried to initialize zero-sized block");
		abort();
#endif
        __builtin_unreachable();
    }

    block->size  = size;
    block->flags = M_BLOCK_FREE;
	block->next  = 0;
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
	(*splitted)->next = src->next;
	src->next = *splitted;

    src->size          = size;

    return 0;
}

static void __mem_merge(){
    mem_block_t* src = heap;
    while(__mem_heap_is_valid_block(src)){
		mem_block_t* next = M_NEXT(src);
        if((src->flags & M_BLOCK_FREE) && __mem_heap_is_valid_block(next) && (next->flags & M_BLOCK_FREE) &&
			(((uint32_t)next) == ((uint32_t)M_MEMORY(src)) + src->size))
		{
            src->size += sizeof(mem_block_t) + next->size;
			src->next = next->next;
        }
        src = M_NEXT(src); 
    }
}

static uint32_t __allocated = 0;
static uint32_t __freed     = 0;
static uint32_t __last_alloc_address = 0;

uint32_t __heap_usage() {
	return __allocated - __freed;
}

void* __attribute__((malloc)) malloc(size_t size){
	LOCK(heap_lock);


    mem_block_t* block            = heap;
	mem_block_t* last_valid_block = heap;
    while(__mem_heap_is_valid_block(block) && (!(block->flags & M_BLOCK_FREE) 
    || (block->size < size 
    || (block->size > size && block->size < size + sizeof(mem_block_t) + 1)))){
		last_valid_block = block;
        block = M_NEXT(block);
    }

    if(!__mem_heap_is_valid_block(block)){
#ifndef __LIBK
		uint32_t grow = (size + sizeof(mem_block_t) + 1) / 0x1000 + 1;
		block = (mem_block_t*) __mem_grow_heap(grow);
		if(!block) {
			fprintf(stderr, "alloc(): out of memory");
			abort();
		}
		heap_size += grow * 0x1000;
		__mem_heap_init_block(block, grow * 0x1000 - sizeof(mem_block_t));
		last_valid_block->next = block;
#else
		cli();
		k_d_mem_heap_print();
		k_panic("Out of memory.", 0);
#endif
    }

	__last_alloc_address = (uint32_t) __builtin_return_address(0);

    if(block->size != size){
        mem_block_t* sblock;
        if(__mem_split_block(block, &sblock, size) != 0){
#ifdef __LIBK
            k_panic("Block split failed. Kmalloc failure.", 0);
#else
			fprintf(stderr, "alloc(): block split failed.");
			abort();
#endif
            __builtin_unreachable();
        }
    }
    
    block->flags &= ~M_BLOCK_FREE;
	__allocated += size;

	UNLOCK(heap_lock);
	return (void*) (M_MEMORY(block));
}

void* __attribute__((malloc)) calloc(size_t num, size_t size){
    void* mem = malloc(num * size);
    memset(mem, 0, num * size);
    return mem;
}

void* __attribute__((malloc)) realloc(void* old, size_t size){
    mem_block_t* hdr = M_HEADER(old);
	if(hdr->size == size) {
		return old;
	}
    void* new_ptr    = malloc(size);
    size_t copy_size = 0;
    if(size > hdr->size){
        copy_size = hdr->size;
    } else {
        copy_size = size;
    }
    memmove(new_ptr, old, copy_size);
    free(old);
    return new_ptr;
}

void free(void* ptr){
	LOCK(heap_lock);
    mem_block_t* header = M_HEADER(ptr);
    if(!__mem_heap_is_valid_block(header) || header->flags & M_BLOCK_FREE){
#ifdef __LIBK
        k_warn("Tried to free invalid pointer: 0x%.8x.", ptr);
		k_warn("0x%.8x", __builtin_return_address(0));
		k_warn("0x%.8x", __builtin_return_address(1));
		k_warn("0x%.8x", __builtin_return_address(2));
#endif
		UNLOCK(heap_lock);
        return;
    }
    header->flags |= M_BLOCK_FREE;
	__freed += header->size;
    __mem_merge();
	UNLOCK(heap_lock);
}

void vfree(void* mem){
    free(((void**)mem)[-1]);
}

void* __attribute__((malloc)) valloc(size_t size, size_t alignment){
	void*  p1; // original block
    void** p2; // aligned block
    int offset = alignment - 1 + sizeof(void*);
    p1 = (void*) malloc(size + offset);
    p2 = (void**)(((size_t)(p1) + offset) & ~(alignment - 1));
    p2[-1] = p1;
    return p2;
}
