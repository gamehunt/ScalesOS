#include <dev/pit.h>
#include <stdio.h>
#include <util/asm_wrappers.h>

#define PIT_SCALE 1193180

void k_dev_pit_init() {
    outb(PIT_MODE, 0x36);  // Mode 3 at channel 0, lo/hi byte access
    k_dev_pit_set_phase(100);
}

void k_dev_pit_set_phase(uint32_t hz) {
	uint32_t divisor = PIT_SCALE / hz;
    cli();
    outb(PIT_CH0_DATA, divisor & 0xFF);  // Low byte
    outb(PIT_CH0_DATA, divisor >> 8);    // High byte
    sti();
}
