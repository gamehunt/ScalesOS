#ifndef __K_DEV_PS2_H
#define __K_DEV_PS2_H

#ifdef __KERNEL
#include "kernel.h"
#else
#include "kernel/kernel.h"
#endif

typedef struct ps2_mouse_packet{
    uint8_t data;
    uint8_t mx;
    uint8_t my;
} ps_mouse_packet_t;

K_STATUS k_dev_ps2_init();

#endif
