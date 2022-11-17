#ifndef __K_DEV_CONSOLE_H
#define __K_DEV_CONSOLE_H

#include <stdint.h>
#include "kernel.h"

typedef void(*console_output_source_t)(char*, uint32_t);

K_STATUS k_dev_console_init();
void     k_dev_console_set_source(console_output_source_t);

#endif