#ifndef __K_INT_SYSCALL_H
#define __K_INT_SYSCALL_H

#include "kernel.h"

#define MAX_SYSCALL 255

typedef uint32_t (*syscall_handler_t)(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e);

K_STATUS k_int_syscall_init();

void     k_int_syscall_setup_handler(uint8_t syscall, syscall_handler_t);

#endif
