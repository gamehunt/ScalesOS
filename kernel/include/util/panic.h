#ifndef _PANIC_H
#define _PANIC_H

#include <int/isr.h>

void k_panic(const char* reason, interrupt_context_t ctx);

#endif