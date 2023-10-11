#ifndef __K_MEM_HEAP_H
#define __K_MEM_HEAP_H

#include <stdint.h>
#include <stdlib.h>
#include "kernel.h"
#include "types.h"

#define k_malloc  malloc
#define k_calloc  calloc
#define k_valloc  valloc
#define k_realloc realloc
#define k_free    free
#define k_vfree   vfree

void*   k_vallocp(uint32_t size, uint32_t alignment, paddr_t* physical);

K_STATUS k_mem_heap_init();
void     k_d_mem_heap_print();

#endif
