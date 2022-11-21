#ifndef __K_DEV_RTC_H
#define __K_DEV_RTC_H

#include "kernel.h"

typedef struct{
    uint8_t  hour, minute, second;
    uint8_t  day, month;
    uint16_t year;
}rtc_time_t;

K_STATUS k_dev_rtc_init();
void     k_dev_rtc_gettime(rtc_time_t* t);
void     k_dev_rtc_enable_centrury();

#endif