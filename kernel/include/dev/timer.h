#ifndef __K_DEV_TIMER_H
#define __K_DEV_TIMER_H

#include "int/isr.h"
#include "kernel.h"
#include <time.h>

typedef void (*timer_callback_t)(interrupt_context_t*);

K_STATUS k_dev_timer_init();
void     k_dev_timer_add_callback(timer_callback_t callback);

uint64_t k_dev_timer_read_tsc();
uint64_t k_dev_timer_get_core_speed();
uint64_t k_dev_timer_tsc_base();
uint64_t k_dev_timer_get_initial_timestamp();

time_t   k_dev_timer_now();

#define now() k_dev_timer_now()

#endif
