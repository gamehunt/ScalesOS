#ifndef __K_PROC_PROCESS_H
#define __K_PROC_PROCESS_H

#include <stdint.h>
#include "int/isr.h"

typedef struct process {
    char name[256];
    interrupt_context_t context;
    uint32_t page_directory;
    uint32_t kernel_stack;
} process_t;

void k_proc_process_setup_context(process_t* process, interrupt_context_t* ctx);
void k_proc_process_save_context(process_t* process, interrupt_context_t ctx);
void k_proc_process_init();

#endif