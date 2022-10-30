#ifndef __K_KERNEL_H
#define __K_KERNEL_H

#define K_STATUS_OK 0
#define K_STATUS_ERR_GENERIC 1

#include <stdint.h>

typedef uint16_t K_STATUS;

#define IS_OK(status) (status == K_STATUS_OK)
#define IS_ERR(status) (status != K_STATUS_OK)

#define EXTEND(mem, count, size) \
    count++; \
    if(count == 1){ \
        mem = k_malloc(size);\
    }else{\
        mem = k_realloc(mem, size * count);\
    }

#define EXPORT(f) void __##f##_exported() __attribute__ ((weak, alias (#f)));

#endif