#include "mem/paging.h"
#include "util/log.h"
#include <proc/smp.h>

typedef struct proc_data {
    uint8_t id;
    uint8_t apic_id;
} proc_data_t;

static proc_data_t cores[64];
static uint8_t     total_cores = 0;
static uint32_t    lapic_addr;

void lapic_write(uint32_t addr, uint32_t value) {
	*((volatile uint32_t*)(lapic_addr + addr)) = value;
}

uint32_t lapic_read(uint32_t addr) {
	return *((volatile uint32_t*)(lapic_addr + addr));
}

extern inline uint64_t __k_proc_smp_read_tsc();

K_STATUS k_proc_smp_init(){
    k_info("Initializing SMP with %d cores", total_cores);

    lapic_addr = (uint32_t) k_mem_paging_map_mmio(lapic_addr, 1);

    k_info("Mapped LAPIC to 0x%.8x %lld %lld", lapic_addr, __k_proc_smp_read_tsc(), __k_proc_smp_read_tsc());

    //TODO we need to proper calibrate our clocks before doing this.

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