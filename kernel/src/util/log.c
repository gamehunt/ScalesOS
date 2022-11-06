#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <util/log.h>
#include "mem/heap.h"

static uint8_t save_logs = 1;
static log_entry_t logs[128];
static uint32_t logs_count = 0;

static uint32_t fg = 0xFFFFFFFF;
static uint32_t bg = 0x00000000;

static void __k_util_log_vprintf_prefixed(const char* prefix, const char* format, va_list va){
    char buffer[4096];
    sprintf(buffer, "%s%s\r\n", prefix, format);
    if(save_logs){
        char log[4096];
        vsnprintf(log, 4096, buffer, va);
        strcpy(logs[logs_count].text, log);
        logs[logs_count].bg = bg;
        logs[logs_count].fg = fg;
        logs_count++;
    }
    vprintf(buffer, va);
}

void k_util_log_info(const char* format, ...){
    va_list va;
    va_start(va, format);
    uint32_t _fg, _bg;
    k_util_log_get_colors(&_fg, &_bg);
    k_util_log_set_colors(0xFFFFFFFF, 0x0);
    __k_util_log_vprintf_prefixed("[I] ", format, va);
    k_util_log_set_colors(_fg, _bg);
    va_end(va);
}

void k_util_log_warn(const char* format, ...){
    va_list va;
    va_start(va, format);
    uint32_t _fg, _bg;
    k_util_log_get_colors(&_fg, &_bg);
    k_util_log_set_colors(0xFF8100, 0x0);
    __k_util_log_vprintf_prefixed("[W] ", format, va);
    k_util_log_set_colors(_fg, _bg);
    va_end(va);
}

void k_util_log_err (const char* format, ...){
    va_list va;
    va_start(va, format);
    uint32_t _fg, _bg;
    k_util_log_get_colors(&_fg, &_bg);
    k_util_log_set_colors(0xFF0000, 0x0);
    __k_util_log_vprintf_prefixed("[E] ", format, va);
    k_util_log_set_colors(_fg, _bg);
    va_end(va);
}

void k_util_log_debug (const char* format, ...){
#ifdef DEBUG
    va_list va;
    va_start(va, format);
    uint32_t _fg, _bg;
    k_util_log_get_colors(&_fg, &_bg);
    k_util_log_set_colors(0xFFFF00, 0x0);
    __k_util_log_vprintf_prefixed("[D] ", format, va);
    k_util_log_set_colors(_fg, _bg);
    va_end(va);
#endif
}

void k_util_log_disable_log_saving(){
    save_logs = 0;
}

uint32_t k_util_log_get_logs(log_entry_t* buffer){
    for(uint32_t i = 0; i < logs_count; i++){
        buffer[i] = logs[i];
    }
    return logs_count;
}

void  k_util_log_set_colors(uint32_t f, uint32_t b){
    fg = f;
    bg = b;
}

void  k_util_log_get_colors(uint32_t* f, uint32_t* b){
    *f = fg;
    *b = bg;
}