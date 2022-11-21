#include "dev/timer.h"
#include "mem/memory.h"
#include "mem/paging.h"
#include "util/log.h"
#include <proc/smp.h>

#include <cpuid.h>
#include <string.h>

typedef struct proc_data {
    uint8_t id;
    uint8_t apic_id;
} proc_data_t;

static proc_data_t cores[64];
static uint8_t     total_cores = 0;
static uint32_t    lapic_addr;

extern void* _binary_smp_bin_start;
extern void* _binary_smp_bin_end;

void __k_proc_smp_lapic_write(uint32_t addr, uint32_t value) {
	*((volatile uint32_t*)(lapic_addr + addr)) = value;
}

uint32_t __k_proc_smp_lapic_read(uint32_t addr) {
	return *((volatile uint32_t*)(lapic_addr + addr));
}

void __k_proc_smp_lapic_send_ipi(int i, uint32_t val) {
	__k_proc_smp_lapic_write(0x310, i << 24);
	__k_proc_smp_lapic_write(0x300, val);
	do { asm volatile ("pause" : : : "memory"); } while (__k_proc_smp_lapic_read(0x300) & (1 << 12));
}

static void __k_proc_smp_delay(unsigned long amount) {
	uint64_t clock = k_dev_timer_read_tsc();
   uint64_t speed = k_dev_timer_get_core_speed();
	while (k_dev_timer_read_tsc() < clock + amount * speed);
}

K_STATUS k_proc_smp_init(){
    k_info("Initializing SMP with %d cores", total_cores);

    lapic_addr = (uint32_t) k_mem_paging_map_mmio(lapic_addr, 1);

    int ebx, unused;
    __cpuid(0, unused, ebx, unused, unused);

    if(ebx == signature_INTEL_ebx){
        k_info("We are on Intel");
    }else if(ebx == signature_AMD_ebx){
        k_info("We are on AMD");
    }else{
        k_info("We are on something, that is not Intel nor AMD");
    }

    uint32_t size = ((uintptr_t)&_binary_smp_bin_end - (uintptr_t)&_binary_smp_bin_start);

    k_mem_paging_map_region(AP_BOOTSTRAP_MAP, 0x1000, size / 0x1000 + 1, 0x3, 1);
    memcpy((void*) AP_BOOTSTRAP_MAP, &_binary_smp_bin_start, size);

    for(uint32_t i = 0; i < (size / 0x1000 + 1);i ++){
        k_mem_paging_unmap(AP_BOOTSTRAP_MAP + i);
    }

    for(int i = 1; i < total_cores; i++){
        k_info("Now initializing core %d...", i);
        __k_proc_smp_lapic_send_ipi(cores[i].apic_id, 0x4500);
		__k_proc_smp_delay(5000UL);
		__k_proc_smp_lapic_send_ipi(cores[i].apic_id, 0x4601);
    }

    return K_STATUS_OK;
}

void  k_proc_smp_set_lapic_addr(uint32_t la){
    lapic_addr = la;
}

void k_proc_smp_add_cpu(uint8_t cpu_id, uint8_t apic_id){
    if(total_cores >= 64){
        k_warn("Core skipped: unsupported amount reached");
        return;
    }

    cores[total_cores].id = cpu_id;
    cores[total_cores].apic_id = apic_id;

    total_cores++;

    k_info("Found core: 0x%.2x 0x%.2x");
}