#ifndef __K_UTIL_LOG_H
#define __K_UTIL_LOG_H

#include <kernel.h>

#define k_info  k_util_log_info
#define k_warn  k_util_log_warn
#define k_err   k_util_log_err
#define k_debug k_util_log_debug

void k_util_log_info(const char* format, ...);
void k_util_log_warn(const char* format, ...);
void k_util_log_err(const char* format, ...);
void k_util_log_debug(const char* format, ...);

#endif