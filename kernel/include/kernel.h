#ifndef __K_KERNEL_H
#define __K_KERNEL_H

#define K_STATUS_OK 0
#define K_STATUS_ERR_GENERIC 1

#include <stdint.h>

typedef uint16_t K_STATUS;

#define IS_OK(status) (status == K_STATUS_OK)
#define IS_ERR(status) (status != K_STATUS_OK)

#endif