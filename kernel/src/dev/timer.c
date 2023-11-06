#include "dev/pit.h"
#include "dev/rtc.h"
#include "int/irq.h"
#include "int/isr.h"
#include "int/pic.h"
#include "kernel.h"
#include "proc/process.h"
#include "util/asm_wrappers.h"
#include "util/log.h"
#include <dev/timer.h>
#include <mem/heap.h>

volatile uint32_t counter = 0;

static timer_callback_t* callbacks;
static uint32_t callback_count;

extern uint64_t __k_dev_timer_compute_speed(uint32_t* start_hi, uint32_t* start_lo);
extern uint64_t k_dev_timer_read_tsc();

#define IS_LEAP(year) (!(year % 4) && ((year % 100) || !(year % 400)))

static uint64_t initial_timestamp = 0;
static uint64_t initial_tsc       = 0;
static uint64_t core_speed        = 0;

uint64_t k_dev_timer_get_core_speed(){
    return core_speed;
}

uint64_t k_dev_timer_tsc_base() {
	return initial_tsc;
}

static uint64_t __k_dev_timer_month_to_timestamp(int month, int year){
    static const int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    uint64_t days = 0;
    for(int i = 1; i <= month; i++){
        days += days_in_month[i - 1];
        if(i == 2 && IS_LEAP(year)){
            days += 1;
        }
    }
    return days * 86400;
}

static uint64_t __k_dev_timer_year_to_timestamp(int year){
    uint64_t days = 0;
    for(int i = 1970; i < year; i++){
        if(IS_LEAP(i)){
            days += 366;
        }else{
            days += 365;
        }
    }
    return days * 86400;
}

static uint64_t __k_dev_timer_to_timestamp(rtc_time_t t){
    uint64_t ls = t.second;
    uint64_t lm = t.minute;
    uint64_t lh = t.hour;
    uint64_t ld = t.day - 1;
    uint64_t r = ls + lm * 60 + lh * 3600 + ld * 86400;
    r += __k_dev_timer_month_to_timestamp(t.month - 1, t.year);
    r += __k_dev_timer_year_to_timestamp(t.year);
    return r;
}

static interrupt_context_t* __irq0_handler(interrupt_context_t* registers){
    if(counter){
        counter--;
    }

    for(uint32_t i = 0; i < callback_count; i++){
        callbacks[i](registers);
    }

    k_int_pic_eoi(0);

    k_proc_process_yield();

    return registers;
}

void k_dev_timer_add_callback(timer_callback_t callback){
    EXTEND(callbacks, callback_count, sizeof(timer_callback_t));
    callbacks[callback_count - 1] = callback;
}

K_STATUS k_dev_timer_init(){
    k_dev_rtc_init();
    k_dev_pit_init();

    cli();

    k_info("Calibrating clocks...");

    uint32_t s_hi, s_lo;

    core_speed = __k_dev_timer_compute_speed(&s_hi, &s_lo) / 10000;

    initial_tsc = s_hi;
    initial_tsc = (initial_tsc << 32) | s_lo;

	initial_tsc /= core_speed;

    rtc_time_t t;
    k_dev_rtc_gettime(&t);
    initial_timestamp = __k_dev_timer_to_timestamp(t);

    callback_count = 0;

    k_int_irq_setup_handler(0, __irq0_handler);
    k_int_pic_unmask_irq(0);

    sti();

    return K_STATUS_OK;
}

time_t k_dev_timer_now() {
	rtc_time_t time;
	k_dev_rtc_gettime(&time);
	return (time_t) __k_dev_timer_to_timestamp(time);
}

uint64_t k_dev_timer_get_initial_timestamp() {
	return initial_timestamp;
}
