#ifndef __PIT_H
#define __PIT_H

#define PIT_CH0_DATA 0x40
#define PIT_CH1_DATA 0x41
#define PIT_CH2_DATA 0x42
#define PIT_MODE     0x43

#include <stdint.h>

void k_dev_pit_init();
void k_dev_pit_set_phase(uint32_t phase);

#endif