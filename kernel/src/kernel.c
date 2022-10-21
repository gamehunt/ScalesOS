#include <multiboot.h>
#include <shared.h>
#include <stdio.h>

#include <util/asm_wrappers.h>
#include <dev/serial.h>
#include <mem/gdt.h>
#include <int/pic.h>
#include <int/idt.h>
#include <int/isr.h>

void kernel_main(uint32_t magic UNUSED, multiboot_info_t* mb UNUSED) 
{
	k_dev_serial_init();

	printf("Booting Scales V%s...\n\r", KERNEL_VERSION);

	k_mem_gdt_init();

	k_int_pic_init(0x20, 0x28);
	k_int_idt_init();
	k_int_isr_init();

	halt();
}