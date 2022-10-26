#include <dev/pit.h>
#include <stdio.h>
#include <util/asm_wrappers.h>

void k_dev_pit_init() {
    outb(PIT_MODE, 0x3C);  // Mode 3 at channel 0, lo/hi byte access
    k_dev_pit_set_phase(1000);
}

void k_dev_pit_set_phase(uint32_t phase) {
    cli();
    outb(PIT_CH0_DATA, phase & 0xFF);           // Low byte
    outb(PIT_CH0_DATA, (phase & 0xFF00) >> 8);  // High byte
    sti();
}