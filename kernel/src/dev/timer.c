#include "int/irq.h"
#include "int/isr.h"
#include "int/pic.h"
#include "kernel.h"
#include "util/asm_wrappers.h"
#include "shared.h"
#include "util/log.h"
#include <dev/timer.h>


volatile uint32_t counter = 0;


static void __irq0_handler(interrupt_context_t registers UNUSED){
    if(counter){
        counter--;
    }
    k_int_pic_eoi(0);
}

K_STATUS k_dev_timer_init(){
	k_int_pic_unmask_irq(0);
    k_int_irq_setup_handler(0, __irq0_handler);
    return K_STATUS_OK;
}

void     k_dev_timer_sleep(uint32_t msec){
    counter = msec;
    while(counter){
        halt();
    }
}