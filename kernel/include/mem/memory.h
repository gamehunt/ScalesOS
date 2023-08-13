#ifndef __K_MEM_MEMORY_H
#define __K_MEM_MEMORY_H

#include "shared.h"

#define KERNEL_STACK_SIZE KB(16)

#define USER_STACK_SIZE   KB(64)
#define USER_STACK_START  0x9000000

#define USER_HEAP_START        USER_STACK_START + USER_STACK_SIZE
#define USER_HEAP_INITIAL_SIZE MB(4)

#define HEAP_SIZE          MB(32)
#define HEAP_START         0xC1000000
#define HEAP_END           HEAP_START + HEAP_SIZE

#define MMIO_START         HEAP_END

#define FRAMEBUFFER_START  0xD0000000

#define MODULES_START      0xE0000000
// #define AP_BOOTSTRAP_MAP   0xEFFF0000
#define PT_TMP_MAP         0xEFFFE000
#define PG_TMP_MAP         0xEFFFF000
#define ACPI_MAPPING_START 0xF0000000

#endif
