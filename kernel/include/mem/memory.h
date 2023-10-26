#ifndef __K_MEM_MEMORY_H
#define __K_MEM_MEMORY_H

#include <shared.h>
#include <stdint.h>

#define USABLE_MEMORY_END      0xFFC00000 // Recursive mappings start
										  
#define KERNEL_STACK_SIZE      KB(32)

#define USER_STACK_SIZE        KB(64)
#define USER_STACK_START       0xBFFFF000
#define USER_STACK_END         (USER_STACK_START - USER_STACK_SIZE)
#define USER_HEAP_INITIAL_SIZE KB(4)
#define USER_MMAP_START        0x40000000

#define BIG_ALLOCATION         KB(128)

#define LOWMEM_START           0xC1000000
#define LOWMEM_END             0xF7000000             
#define LOWMEM_SIZE            (LOWMEM_END - LOWMEM_START)

#define HEAP_START             (LOWMEM_END + 0x1000)
#define HEAP_END               (USABLE_MEMORY_END - 0x1000)             
#define HEAP_SIZE              (HEAP_END - HEAP_START)
#define HEAP_INITIAL_SIZE      KB(16)

void* k_map(uint32_t frame, uint32_t size, uint8_t flags);
void  k_unmap(void* addr, uint32_t size);

#endif
