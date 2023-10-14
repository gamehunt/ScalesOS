#ifndef __SYS_HEAP_H
#define __SYS_HEAP_H

#include <stddef.h>
#include <stdint.h>

struct __attribute__((packed)) mem_block {
    size_t            size;
    uint8_t           flags;
	struct mem_block* next;
};

typedef struct mem_block mem_block_t;

#define M_BLOCK_FREE    (1 << 0)
#define M_BLOCK_ALIGNED (1 << 1)

//mem_block_t* M_HEADER(void* addr);
#define M_HEADER(addr)     ((mem_block_t*)(((uint32_t)addr) - sizeof(mem_block_t)))    
//void* M_MEMORY(mem_header_t* addr);     
#define M_MEMORY(addr)     (((uint32_t)addr) + sizeof(mem_block_t))
//mem_block_t* M_NEXT(mem_block_t* addr);
#define M_NEXT(addr)       (addr->next) 
#endif
