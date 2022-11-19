#ifndef __K_MEM_MEMORY_H
#define __K_MEM_MEMORY_H

#define HEAP_SIZE     MB(32)
#define HEAP_START    0xC1000000
#define HEAP_END      HEAP_START + HEAP_SIZE

#define MMIO_START    HEAP_END

#define PT_TMP_MAP         0xEFFFE000
#define PG_TMP_MAP         0xEFFFF000
#define ACPI_MAPPING_START 0xF0000000

#endif