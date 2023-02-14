#include "dev/timer.h"
#include "mem/heap.h"
#include "mem/memory.h"
#include "mem/paging.h"
#include "util/log.h"
#include <proc/smp.h>

#include <cpuid.h>
#include <string.h>
#include <stdio.h>

typedef struct proc_data {
    uint8_t id;
    uint8_t apic_id;
} proc_data_t;

static proc_data_t cores[64];
static uint8_t total_cores = 0;
static uint32_t lapic_addr;

extern void* __k_proc_smp_ap_trampoline;
uintptr_t __k_proc_smp_stack;

void __k_proc_smp_lapic_write(uint32_t addr, uint32_t value) {
    *((volatile uint32_t*)(lapic_addr + addr)) = value;
}

uint32_t __k_proc_smp_lapic_read(uint32_t addr) {
    return *((volatile uint32_t*)(lapic_addr + addr));
}

void __k_proc_smp_lapic_send_ipi(int i, uint32_t val) {
    __k_proc_smp_lapic_write(0x310, i << 24);
    __k_proc_smp_lapic_write(0x300, val);
    do {
        asm volatile("pause" : : : "memory");
    } while (__k_proc_smp_lapic_read(0x300) & (1 << 12));
}

static void __k_proc_smp_delay(unsigned long amount) {
    uint64_t clock = k_dev_timer_read_tsc();
    uint64_t speed = k_dev_timer_get_core_speed();
    while (k_dev_timer_read_tsc() < clock + amount * speed)
        ;
}

K_STATUS k_proc_smp_init() {
    k_info("Initializing SMP with %d cores", total_cores);

    lapic_addr = (uint32_t)k_mem_paging_map_mmio(lapic_addr, 1);

    int ebx, unused;
    __cpuid(0, unused, ebx, unused, unused);

    if (ebx == signature_INTEL_ebx) {
        k_info("We are on Intel");
    } else if (ebx == signature_AMD_ebx) {
        k_info("We are on AMD");
    } else {
        k_info("We are on something, that is not Intel nor AMD");
    }

    k_mem_paging_map_region(0x1000, 0x1000, 1, 0x3, 1);       // Identity map first page for AP purpose
    memcpy((void*)0x1000, &__k_proc_smp_ap_trampoline, 4096); // TODO: unmap in later in C routine?

    for (int i = 1; i < total_cores; i++) {
        k_info("Now initializing core %d...", i);
        __k_proc_smp_stack = ((uintptr_t)k_valloc(KB(16), 16)) + KB(16);
        k_info("Core stack = 0x%.8x", __k_proc_smp_stack);

        __k_proc_smp_lapic_send_ipi(cores[i].apic_id, 0x4500);
        __k_proc_smp_delay(5000UL);
        __k_proc_smp_lapic_send_ipi(cores[i].apic_id, 0x4601);
    }

    while(1);

    return K_STATUS_OK;
}

void k_proc_smp_set_lapic_addr(uint32_t la) {
    lapic_addr = la;
}

void k_proc_smp_add_cpu(uint8_t cpu_id, uint8_t apic_id) {
    if (total_cores >= 64) {
        k_warn("Core skipped: unsupported amount reached");
        return;
    }

    cores[total_cores].id = cpu_id;
    cores[total_cores].apic_id = apic_id;

    total_cores++;

    k_info("Found core: 0x%.2x 0x%.2x");
}

void __k_proc_smp_ap_startup() {
    k_info("AP_STARTUP");
    while (1)
        ;
}
