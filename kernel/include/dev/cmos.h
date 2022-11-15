#ifndef __K_DEV_CMOS_H
#define __K_DEV_CMOS_H

#include <stdint.h>

void    k_dev_cmos_nmi_enable();
void    k_dev_cmos_nmi_disable();

uint8_t k_dev_cmos_read(uint8_t reg);
void    k_dev_cmos_write(uint8_t reg, uint8_t val);

#endif