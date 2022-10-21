#include <multiboot.h>
#include <shared.h>
#include <stdio.h>

#include <util/asm_wrappers.h>

#include <dev/serial.h>
#include <dev/pit.h>

#include <mem/gdt.h>
#include <mem/paging.h>

#include <int/pic.h>
#include <int/idt.h>
#include <int/isr.h>
#include <int/irq.h>

void kernel_main(uint32_t magic UNUSED, multiboot_info_t* mb UNUSED) 
{
	k_dev_serial_init();

	printf("Booting Scales V%s...\n\r", KERNEL_VERSION);

	k_mem_gdt_init();

	k_int_pic_init(0x20, 0x28);
	k_int_idt_init();
	k_int_isr_init();
	k_int_irq_init();

	k_mem_paging_init();

	for(uint32_t i = 0; i < mb->framebuffer_width * mb->framebuffer_width * 4; i+=0x1000){
    	k_mem_paging_map(0xE0000000 + i, mb->framebuffer_addr + i, 0x3);
	}

	//Just draw something for tests
	for(uint32_t i = 0; i < mb->framebuffer_height; i++){
		*(((uint32_t*)0xE0000000) + i*mb->framebuffer_width + (mb->framebuffer_width / 4))      = 0x00AA00;
		*(((uint32_t*)0xE0000000) + i*mb->framebuffer_width + (mb->framebuffer_width / 2))      = 0x00AA00;
		*(((uint32_t*)0xE0000000) + i*mb->framebuffer_width + (mb->framebuffer_width * 3 / 4))  = 0x00AA00;
	}

	for(uint32_t i = 0; i < mb->framebuffer_width; i++){
		*(((uint32_t*)0xE0000000) + (mb->framebuffer_height / 2)*mb->framebuffer_width + i)     = 0xAA0000;
		*(((uint32_t*)0xE0000000) + (mb->framebuffer_height / 4)*mb->framebuffer_width + i)     = 0x0000AA;
		*(((uint32_t*)0xE0000000) + (mb->framebuffer_height * 3 / 4)*mb->framebuffer_width + i) = 0x0000AA;
	}

	k_int_pic_unmask_irq(0);
	k_dev_pit_init();

	while(1){
		asm("hlt");
	}
}