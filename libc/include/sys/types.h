#ifndef __SYS_TYPES_H
#define __SYS_TYPES_H

#include <stdint.h>

#include "kernel/types.h"

typedef int dev_t;
typedef int ino_t;
typedef int mode_t;

typedef long clock_t;
typedef long time_t;
typedef long msec_t;

typedef uint32_t tcflag_t;
typedef uint32_t speed_t;
typedef uint8_t  cc_t;     

typedef uint32_t off_t;

typedef unsigned long blksize_t;
typedef unsigned long blkcnt_t;

typedef unsigned short nlink_t;

#endif
