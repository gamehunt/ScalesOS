#ifndef __K_UTIL_LOG_H
#define __K_UTIL_LOG_H

#include <kernel.h>

#define k_info  k_util_log_info
#define k_warn  k_util_log_warn
#define k_err   k_util_log_err
#define k_debug k_util_log_debug

typedef struct{
    char text[4096];
    uint32_t fg;
    uint32_t bg;
} log_entry_t;

void k_util_log_info(const char* format, ...);
void k_util_log_warn(const char* format, ...);
void k_util_log_err(const char* format, ...);
void k_util_log_debug(const char* format, ...);

uint32_t k_util_log_get_logs(log_entry_t* buffer);
void     k_util_log_disable_log_saving();

void     k_util_log_set_colors(uint32_t fg, uint32_t bg);
void     k_util_log_get_colors(uint32_t* fg, uint32_t* bg);
#endif