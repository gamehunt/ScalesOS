#include "kernel.h"
#include "util/asm_wrappers.h"
#include <dev/cmos.h>

uint8_t nmi_enabled = 1;

void k_dev_cmos_nmi_enable(){
    outb(0x70, inb(0x70) & 0x7F);
    inb(0x71);

    nmi_enabled = 1;
}

void k_dev_cmos_nmi_disable(){
    outb(0x70, inb(0x70) | 0x80);
    inb(0x71);

    nmi_enabled = 0;
}

static void __k_dev_cmos_select_register(uint8_t r){
    outb(0x70, (nmi_enabled << 7) | r);
}

uint8_t k_dev_cmos_read(uint8_t reg){
    __k_dev_cmos_select_register(reg);
    return inb(0x71);
}

void    k_dev_cmos_write(uint8_t reg, uint8_t val){
    __k_dev_cmos_select_register(reg);
    outb(0x71, val);
}