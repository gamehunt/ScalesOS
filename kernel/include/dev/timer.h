#ifndef __TIMER_H
#define __K_DEV_TIMER_H

#include "kernel.h"

#define k_sleep k_dev_timer_sleep

K_STATUS k_dev_timer_init();
void k_dev_timer_sleep(uint32_t msec);

#endif