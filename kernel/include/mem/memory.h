#ifndef __K_MEM_MEMORY_H
#define __K_MEM_MEMORY_H

#include "shared.h"

#define HEAP_SIZE     MB(32)
#define HEAP_START    0xC1000000
#define HEAP_END      HEAP_START + HEAP_SIZE

#define MMIO_START    HEAP_END

#define FB_MAP             0xD0000000

// #define AP_BOOTSTRAP_MAP   0xEFFF0000
#define PT_TMP_MAP         0xEFFFE000
#define PG_TMP_MAP         0xEFFFF000
#define ACPI_MAPPING_START 0xF0000000

#endif
