#ifndef __K_MEM_MEMORY_H
#define __K_MEM_MEMORY_H

#include "shared.h"

#define KERNEL_STACK_SIZE KB(32)

#define USER_STACK_SIZE        KB(64)
#define USER_STACK_START       0xBFFFF000
#define USER_STACK_END         (USER_STACK_START - USER_STACK_SIZE)
#define USER_HEAP_INITIAL_SIZE KB(4)
#define USER_MMAP_START        0x40000000

#define BIG_ALLOCATION         KB(128)

#define HEAP_SIZE          MB(32)
#define HEAP_START         0xC1000000
#define HEAP_END           (HEAP_START + HEAP_SIZE)

#define MMIO_SIZE          MB(4)
#define MMIO_START         (HEAP_END + 0x1000)
#define MMIO_END           (MMIO_START + MMIO_SIZE)

#define FRAMEBUFFER_START  (MMIO_END + 0x1000)

// #define AP_BOOTSTRAP_MAP   0xEFFF0000
#define PT_TMP_MAP         0xEFFFE000
#define PG_TMP_MAP         0xEFFFF000
#define ACPI_MAPPING_START 0xF0000000

#endif
