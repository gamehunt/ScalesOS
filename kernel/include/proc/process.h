#ifndef __K_PROC_PROCESS_H
#define __K_PROC_PROCESS_H

#include "fs/vfs.h"
#include "int/isr.h"
#include "mem/mmap.h"
#include "mem/paging.h"
#include "proc/spinlock.h"
#include "sys/signal.h"
#include "signal.h"
#include "types.h"
#include "util/types/list.h"
#include "util/types/tree.h"
#include <stdint.h>

#define PROCESS_STATE_STARTING           0x0
#define PROCESS_STATE_RUNNING            0x1
#define PROCESS_STATE_FINISHED           0x2
#define PROCESS_STATE_SLEEPING           0x3

#define IS_READY(state) \
	(state < PROCESS_STATE_FINISHED)

#define PROCESS_WAITPID_WNOHANG          (1 << 0)
#define PROCESS_WAITPID_WUNTRACED        (1 << 1)

#define PROCESS_FLAG_TASKLET             (1 << 0)

#define IS_SIGNAL_IGNORED(proc, sig) \
	(proc->signals_ignored & (1 << sig))

#define SET_SIGNAL_IGNORED(proc, sig) \
	 proc->signals_ignored |= (1 << sig);

#define SET_SIGNAL_UNIGNORED(proc, sig) \
	 proc->signals_ignored &= ~(1 << sig);

typedef struct context {
    uint32_t esp;                                     // +0
    uint32_t ebp;                                     // +4
    uint32_t eip;                                     // +8
	uint8_t* fp_regs;
} context_t;

typedef struct image{
    pde_t*      page_directory;
	uint32_t    heap;
	uint32_t    heap_size;
    uint32_t    kernel_stack;
	uint32_t*   kernel_stack_base;
	uint32_t    user_stack;
	uint32_t    entry;
	mmap_info_t mmap_info;
} image_t;

typedef struct fd {
	fs_node_t* node;
	uint32_t   offset;
	uint32_t   links;
} fd_t;

typedef struct fd_list {
	fd_t**      nodes;
	uint32_t    amount;
	uint32_t    size;
	spinlock_t  lock;
} fd_list_t;

typedef struct signal {
	signal_handler_t handler;
	uint8_t          flags;
} signal_t;

struct wait_node;
struct block_node;

typedef struct process {
    char      name[256];
    pid_t     pid;
	uid_t     uid;
	int       status;
	group_t   group_id;
    context_t context;
    image_t   image;
    interrupt_context_t syscall_state;
    uint32_t  flags;
    uint8_t   state;
	fd_list_t fds;

	tree_node_t* node;

	struct wait_node*  wait_node;
	list_t*            wait_queue;

	struct block_node* block_node;

	uint64_t   time_total;
	uint64_t   time_system;

	uint64_t   last_entrance;
	uint64_t   last_sys;

	signal_t   signals[MAX_SIGNAL];
	uint64_t   signal_queue;
	uint64_t   signals_ignored;

	uint32_t   interrupted_syscall;

	fs_node_t* wd_node;
	char       wd[256];

	fs_node_t* select_wait_node;
	uint8_t    select_wait_event;
	struct     wait_node* timeout_node;
} process_t;

typedef struct {
	uint8_t    is_valid;
	process_t* process;
	uint8_t    interrupted;
} generic_sleep_node_t;

typedef struct wait_node {
	generic_sleep_node_t data;
	uint64_t   seconds;
	uint64_t   microseconds;
	struct wait_node* next;
} wait_node_t;

typedef struct block_node {
	generic_sleep_node_t data;
	uint32_t   links;
} block_node_t;

void       k_proc_process_yield();
void       k_proc_process_switch() __attribute__((noreturn));
void       k_proc_process_update_timings();
void       k_proc_process_init();
uint32_t   k_proc_process_fork();
void       k_proc_process_init_core();

void       k_proc_process_mark_ready(process_t* process);

process_t* k_proc_process_current();
process_t* k_proc_process_next();
list_t*    k_proc_process_list();

uint32_t   k_proc_process_open_node(process_t* process, fs_node_t* node);
void       k_proc_process_close_fd(process_t* process, uint32_t fd);

void*      k_proc_process_grow_heap(process_t* process, int32_t size);

uint32_t   k_proc_process_sleep(process_t* process, uint64_t microseconds);
void       k_proc_process_timeout(process_t* process, uint64_t seconds, uint64_t microseconds);
uint8_t    k_proc_process_sleep_on_queue(process_t* process, list_t* queue);
uint8_t    k_proc_process_sleep_and_unlock(process_t* process, list_t* queue, spinlock_t* lock);
pid_t      k_proc_process_waitpid(process_t* process, int pid, int* status, int options);
void       k_proc_process_wakeup_queue(list_t* queue);
void       k_proc_process_wakeup_queue_single(list_t* queue);
void       k_proc_process_wakeup_queue_single_select(list_t* queue, fs_node_t* fsnode, uint8_t event);
void       k_proc_process_wakeup_queue_select(list_t* queue, fs_node_t* node, uint8_t event);
void       k_proc_process_wakeup_on_signal(process_t* process); 

void       k_proc_process_exit(process_t* process, int code) __attribute__((noreturn));
void       k_proc_process_destroy(process_t* process);

uint8_t    k_proc_process_is_ready(process_t* process);
void       k_proc_process_send_signal(process_t* process, int sig);
void       k_proc_process_process_signals(process_t* process, interrupt_context_t* ctx);
void       k_proc_process_return_from_signal(interrupt_context_t* ctx);

process_t* k_proc_process_find_by_pid(pid_t pid);

pid_t      k_proc_process_create_tasklet(const char* name, uintptr_t entry, void* data);

#endif
