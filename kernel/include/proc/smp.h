#ifndef __K_PROC_SMP_H
#define __K_PROC_SMP_H

#include "kernel.h"

struct gdt_entry;
struct tss_entry;

typedef struct {
    struct gdt_entry* gdt;
    struct tss_entry* tss;

    uint32_t page_directory;
} core_t;

static core_t __seg_gs *current_core = 0;

K_STATUS k_proc_smp_init();
void     k_proc_smp_add_cpu(uint8_t cpu_id, uint8_t apic_id);
void     k_proc_smp_set_lapic_addr(uint32_t lapic_addr);

#endif
