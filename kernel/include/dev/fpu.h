#ifndef __K_DEV_FPU_H
#define __K_DEV_FPU_H

#include "kernel.h"
#include "proc/process.h"

K_STATUS k_dev_fpu_init();

void     k_dev_fpu_save(process_t* process);
void     k_dev_fpu_restore(process_t* process);

#endif
