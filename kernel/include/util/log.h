#ifndef __K_UTIL_LOG_H
#define __K_UTIL_LOG_H

#include <kernel.h>

void k_info(const char* format, ...);
void k_warn(const char* format, ...);
void k_err(const char* format, ...);
void k_debug(const char* format, ...);

#endif