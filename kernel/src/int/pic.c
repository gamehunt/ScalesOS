
#include "util/log.h"
#include <int/pic.h>

#include <util/asm_wrappers.h>

void k_int_pic_eoi(uint8_t irq){
	if(irq >= 8){
		outb(PIC2_COMMAND,PIC_EOI);
    }
 
	outb(PIC1_COMMAND,PIC_EOI);
}

void k_int_pic_init(uint8_t offset1, uint8_t offset2)
{
    cli();
 
	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	outb(PIC1_DATA, offset1);                 // ICW2: Master PIC vector offset
	outb(PIC2_DATA, offset2);                 // ICW2: Slave PIC vector offset
	outb(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	outb(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
 
	outb(PIC1_DATA, ICW4_8086);
	outb(PIC2_DATA, ICW4_8086);

    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);

    sti();
}

void k_int_pic_mask_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;
 
    if(irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = inb(port) | (1 << irq);
    outb(port, value);        
}
 
void k_int_pic_unmask_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;
 
    if(irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    value = inb(port) & ~(1 << irq);
    outb(port, value);        
}