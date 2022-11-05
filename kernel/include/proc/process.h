#ifndef __K_PROC_PROCESS_H
#define __K_PROC_PROCESS_H

#include <stdint.h>

typedef struct context {
    uint32_t esp;            // +0
    uint32_t ebp;            // +4
    uint32_t eip;            // +8
    uint32_t page_directory; // +12
}context_t;

typedef struct process {
    char      name[256];
    uint32_t  pid;
    context_t context;
} process_t;

void k_proc_process_yield();
void k_proc_process_init();

#endif