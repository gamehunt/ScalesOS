#ifndef __K_UTIL_PANIC_H
#define __K_UTIL_PANIC_H

#include <int/isr.h>

void k_panic(const char* reason, interrupt_context_t* ctx)  __attribute__ ((noreturn));

#endif
