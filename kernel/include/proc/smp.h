#ifndef __K_PROC_SMP_H
#define __K_PROC_SMP_H

#include "kernel.h"

K_STATUS k_proc_smp_init();
void     k_proc_smp_add_cpu(uint8_t cpu_id, uint8_t apic_id);
void     k_proc_smp_set_lapic_addr(uint32_t lapic_addr);

#endif