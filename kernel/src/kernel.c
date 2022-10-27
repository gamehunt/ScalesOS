#include <dev/pit.h>
#include <dev/serial.h>
#include <int/idt.h>
#include <int/irq.h>
#include <int/isr.h>
#include <int/pic.h>
#include <mem/gdt.h>
#include <mem/paging.h>
#include <multiboot.h>
#include <shared.h>
#include <stdio.h>
#include <util/asm_wrappers.h>
#include <util/log.h>

#include "dev/pci.h"
#include "dev/timer.h"
#include "mem/heap.h"
#include "mem/pmm.h"

extern void k_mem_print();

void kernel_main(uint32_t magic UNUSED, multiboot_info_t* mb) {
    k_dev_serial_init();

    k_info("Booting Scales V%s...", KERNEL_VERSION);

    k_mem_gdt_init();

    k_int_pic_init(0x20, 0x28);
    k_int_idt_init();
    k_int_isr_init();
    k_int_irq_init();

    k_mem_pmm_init(mb);
    k_mem_paging_init();
    k_mem_heap_init();

    k_dev_pci_init();

    k_dev_pit_init();
    k_dev_timer_init();

    while (1) {
        asm("hlt");
    }
}