#ifndef __K_DEV_TIMER_H
#define __K_DEV_TIMER_H

#include "int/isr.h"
#include "kernel.h"

#define k_sleep k_dev_timer_sleep

typedef void (*timer_callback_t)(interrupt_context_t*);

K_STATUS k_dev_timer_init();
void     k_dev_timer_add_callback(timer_callback_t callback);
void     k_dev_timer_sleep(uint32_t msec);

uint64_t k_dev_timer_read_tsc();
uint64_t k_dev_timer_get_core_speed();
uint64_t k_dev_timer_tsc_base();
uint64_t k_dev_timer_get_initial_timestamp();

#endif
