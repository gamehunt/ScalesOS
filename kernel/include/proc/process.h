#ifndef __K_PROC_PROCESS_H
#define __K_PROC_PROCESS_H

#include "fs/vfs.h"
#include "int/isr.h"
#include "proc/spinlock.h"
#include "sys/types.h"
#include <stdint.h>

#define PROCESS_STATE_STARTING           0x0
#define PROCESS_STATE_STARTED            0x1
#define PROCESS_STATE_FINISHED           0x2

typedef struct context {
    uint32_t esp;             // +0
    uint32_t ebp;             // +4
    uint32_t eip;             // +8
} context_t;

typedef struct image{
    uint32_t   page_directory;
	uint32_t   heap;
	uint32_t   heap_size;
    uint32_t   kernel_stack;
	uint32_t*  kernel_stack_base;
	uint32_t   user_stack;
	uint32_t   entry;
} image_t;

typedef struct fd_list {
	fs_node_t** nodes;
	uint32_t    amount;
	uint32_t    size;
	spinlock_t  lock;
} fd_list_t;

typedef struct process {
    char      name[256];
    pid_t  pid;
    context_t context;
    image_t   image;
    interrupt_context_t syscall_state;
    uint32_t  flags;
    uint8_t   state;
	fd_list_t fds;
} process_t;

void       k_proc_process_yield();
void       k_proc_process_switch();
void       k_proc_process_init();
void       k_proc_process_exec(const char* path, int argc, char** argv);
uint32_t   k_proc_process_fork();
void       k_proc_process_init_core();

void       k_proc_process_mark_ready(process_t* process);

process_t* k_proc_process_current();
process_t* k_proc_process_next();

uint32_t   k_proc_process_open_node(process_t* process, fs_node_t* node);
void       k_proc_process_close_fd(process_t* process, uint32_t fd);

void       k_proc_process_grow_heap(process_t* process, int32_t size);

void       k_proc_process_exit(process_t* process, int code);

#endif
