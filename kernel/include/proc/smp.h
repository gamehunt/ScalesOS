#ifndef __K_PROC_SMP_H
#define __K_PROC_SMP_H

#include "kernel.h"
#include "proc/process.h"

struct gdt_entry;
struct tss_entry;

typedef struct {
    struct gdt_entry* gdt;
    struct tss_entry* tss;

    volatile process_t* current_process;
    process_t* idle_process;
} core;

static core __seg_gs * const current_core __attribute__((__used__)) = 0;

K_STATUS k_proc_smp_init();
void     k_proc_smp_add_cpu(uint8_t cpu_id, uint8_t apic_id);
void     k_proc_smp_set_lapic_addr(uint32_t lapic_addr);

#endif
