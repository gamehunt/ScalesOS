#ifndef __K_DEV_KERNEL_SERIAL
#define __K_DEV_KERNEL_SERIAL

#define PORT 0x3F8

#include <kernel.h>

K_STATUS k_dev_serial_init();
uint8_t k_dev_serial_received();
char k_dev_serial_read();
uint8_t k_dev_serial_is_transmit_empty();
void k_dev_serial_write(char a);

#endif