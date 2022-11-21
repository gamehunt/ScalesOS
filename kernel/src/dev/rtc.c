#include "dev/cmos.h"
#include "kernel.h"
#include "util/asm_wrappers.h"
#include "util/log.h"
#include <dev/rtc.h>
#include <stdio.h>

static uint8_t h24 = 0;
static uint8_t bin = 0;

static uint8_t century = 0;

void  k_dev_rtc_enable_centrury(){
    century = 1;
    k_info("Enabled century register");
}

static uint8_t __k_dev_rtc_from_bcd(uint8_t bcd){
    return  ((bcd & 0xF0) >> 1) + ( (bcd & 0xF0) >> 3) + (bcd & 0xf);
}

K_STATUS k_dev_rtc_init(){
    cli();
    k_dev_cmos_nmi_disable();

    uint8_t b = k_dev_cmos_read(0xB);

    k_info("RTC Configuration:");

    h24 = b & 0x2;
    bin = b & 0x4;

    if(h24){
        printf("\t-- Format: 24h ");
    }else{
        printf("\t-- Format: 12h ");
    }

    if(bin){
        printf("binary");
    }else{
        printf("bcd");
    }

    printf("\r\n");

    rtc_time_t time;
    k_dev_rtc_gettime(&time);

    k_debug("RTC time: %.2d:%.2d:%.2d %d.%d.%d", time.hour, time.minute, time.second, time.day, time.month, time.year);

    k_dev_cmos_nmi_enable();
    sti();
    return K_STATUS_OK;
}

void k_dev_rtc_gettime(rtc_time_t* t){
    t->second = k_dev_cmos_read(0x0);
    t->minute = k_dev_cmos_read(0x2);
    t->hour   = k_dev_cmos_read(0x4);
    t->day    = k_dev_cmos_read(0x7);
    t->month  = k_dev_cmos_read(0x8);
    t->year   = k_dev_cmos_read(0x9);

    if(!bin){
        t->second = __k_dev_rtc_from_bcd(t->second);
        t->minute = __k_dev_rtc_from_bcd(t->minute);
        t->hour   = __k_dev_rtc_from_bcd(t->hour);
        t->day    = __k_dev_rtc_from_bcd(t->day);
        t->month  = __k_dev_rtc_from_bcd(t->month);
        t->year   = __k_dev_rtc_from_bcd(t->year);  
    }

    if(!h24){
        t->hour = ((t->hour & 0x7F) + 12) % 24;
    }

    if(century){
        uint8_t c = k_dev_cmos_read(0x32);
        if(!bin){
            t->year += __k_dev_rtc_from_bcd(c) * 100;
        }else{
            t->year += c * 100;
        }
    }else{
        t->year += (t->year < 70 ? 2000 : 1900);
    }
}