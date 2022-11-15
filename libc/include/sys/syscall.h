#ifndef __SYS_SYSCALL_H
#define __SYS_SYSCALL_H

#include <stdint.h>

#if !defined(__LIBK) && !defined(__KERNEL)

extern uint32_t __syscall(uint32_t num, uint32_t a, uint32_t b, uint32_t c, uint32_t e, uint32_t f);

#endif

#endif