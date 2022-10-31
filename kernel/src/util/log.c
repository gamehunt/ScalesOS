#include "kernel.h"
#include "stdio.h"
#include <stdarg.h>
#include <util/log.h>

static void vprintf_prefixed(const char* prefix, const char* format, va_list va){
    char buffer[4096];
    sprintf(buffer, "%s%s\r\n", prefix, format);
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