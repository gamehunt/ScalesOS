#include "int/irq.h"
#include "int/isr.h"
#include "int/pic.h"
#include "kernel.h"
#include "util/asm_wrappers.h"
#include <dev/timer.h>
#include <mem/heap.h>

volatile uint32_t counter = 0;

static timer_callback_t* callbacks;
static uint32_t callback_count;

static interrupt_context_t* __irq0_handler(interrupt_context_t* registers){
    if(counter){
        counter--;
    }
    for(uint32_t i = 0; i < callback_count; i++){
        callbacks[i](registers);
    }
    k_int_pic_eoi(0);
    return registers;
}

void k_dev_timer_add_callback(timer_callback_t callback){
    EXTEND(callbacks, callback_count, sizeof(timer_callback_t));
    callbacks[callback_count - 1] = callback;
}

K_STATUS k_dev_timer_init(){
	k_int_pic_unmask_irq(0);
    k_int_irq_setup_handler(0, __irq0_handler);
    callback_count = 0;
    return K_STATUS_OK;
}

void     k_dev_timer_sleep(uint32_t msec){
    counter = msec;
    while(counter){
        halt();
    }
}