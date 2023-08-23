#ifndef __K_TYPES_H
#define __K_TYPES_H

#include <stdint.h>

typedef uintptr_t addr_t;
typedef addr_t    paddr_t;
typedef addr_t    vaddr_t;

typedef addr_t    syscall_param_t;
typedef int32_t   syscall_result_t;

typedef int32_t pid_t;
typedef int32_t uid_t;
typedef int32_t group_t;

#endif
