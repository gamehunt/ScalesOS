#ifndef __K_MEM_HEAP_H
#define __K_MEM_HEAP_H

#include <stdint.h>

#include "kernel.h"

#define kmalloc k_mem_heap_alloc
#define kfree   k_mem_heap_free

K_STATUS k_mem_heap_init();
void*    k_mem_heap_alloc(uint32_t size);
void     k_mem_heap_free(void* ptr);

#endif