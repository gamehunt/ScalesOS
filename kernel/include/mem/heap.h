#ifndef __K_MEM_HEAP_H
#define __K_MEM_HEAP_H

#include <stdint.h>
#include "shared.h"
#include "kernel.h"

#define HEAP_START    0xC1000000
#define HEAP_END      0xCFFFF000
#define HEAP_SIZE     MB(1)

#define k_malloc  k_mem_heap_alloc
#define k_calloc  k_mem_heap_calloc
#define k_valloc  k_mem_heap_valloc
#define k_realloc k_mem_heap_realloc
#define k_free    k_mem_heap_free
#define k_vfree   k_mem_heap_vfree

K_STATUS k_mem_heap_init();
void*    k_mem_heap_alloc(uint32_t size);
void*    k_mem_heap_calloc(uint32_t size, uint8_t fill_value);
void*    k_mem_heap_valloc(uint32_t size, uint32_t alignment);
void*    k_mem_heap_realloc(void* old, uint32_t size);
void     k_mem_heap_free(void* mem);
void     k_mem_heap_vfree(void* mem);
void     k_d_mem_heap_print();

#endif