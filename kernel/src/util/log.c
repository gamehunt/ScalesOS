#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <util/log.h>
#include "mem/heap.h"

static uint8_t save_logs = 1;
static char logs[4096][128];
static uint32_t logs_count = 0;

static void vprintf_prefixed(const char* prefix, const char* format, va_list va){
    char buffer[4096];
    sprintf(buffer, "%s%s\r\n", prefix, format);
    if(save_logs){
        char log[4096];
        vsnprintf(log, 4096, buffer, va);
        strcpy(logs[logs_count], log);
        logs_count++;
    }
    vprintf(buffer, va);
}

void k_info(const char* format, ...){
    va_list va;
    va_start(va, format);
    vprintf_prefixed("[I] ", format, va);
    va_end(va);
}

void k_warn(const char* format, ...){
    va_list va;
    va_start(va, format);
    vprintf_prefixed("[W] ", format, va);
    va_end(va);
}

void k_err (const char* format, ...){
    va_list va;
    va_start(va, format);
    vprintf_prefixed("[E] ", format, va);
    va_end(va);
}

void k_debug (const char* format, ...){
#ifdef DEBUG
    va_list va;
    va_start(va, format);
    vprintf_prefixed("[D] ", format, va);
    va_end(va);
#endif
}

void k_disable_log_saving(){
    save_logs = 0;
}

uint32_t k_get_logs(char** buffer){
    for(uint32_t i = 0; i < logs_count; i++){
        buffer[i] = k_malloc(strlen(logs[i]) + 1);
        strcpy(buffer[i], logs[i]);
    }
    return logs_count;
}