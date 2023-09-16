#ifndef __K_MEM_HEAP_H
#define __K_MEM_HEAP_H

#include <stdint.h>
#include <stdlib.h>
#include "kernel.h"

#define k_malloc  malloc
#define k_calloc  calloc
#define k_valloc  valloc
#define k_realloc realloc
#define k_free    free
#define k_vfree   vfree

K_STATUS k_mem_heap_init();
void     k_d_mem_heap_print();

#endif
