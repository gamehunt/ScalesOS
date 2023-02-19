#include "fs/vfs.h"
#include "int/isr.h"
#include "kernel.h"
#include "mem/gdt.h"
#include "mem/heap.h"
#include "mem/paging.h"
#include "mod/elf.h"
#include "proc/spinlock.h"
#include "util/asm_wrappers.h"
#include "util/log.h"
#include "util/panic.h"
#include "util/types/stack.h"
#include <proc/process.h>
#include <proc/smp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KERNEL_STACK_SIZE KB(16)
#define USER_STACK_SIZE KB(64)
#define USER_STACK_START 0x9000000

#define GUARD_MAGIC 0xBEDAABED

static process_t** processes;
static uint32_t total_processes;
static uint32_t next_process;

static spinlock_t proc_lock = 0;

static uint8_t __k_proc_process_check_stack(process_t* proc) {
    if (proc->pid == 1) {
        return 1;
    }
    return proc->image.kernel_stack[0] == GUARD_MAGIC;
}

static void __k_proc_process_create_kernel_stack(process_t* proc) {
    uint32_t* stack = k_valloc(KERNEL_STACK_SIZE, 16);
    memset(stack, 0, KERNEL_STACK_SIZE);
    stack[0] = GUARD_MAGIC;
    proc->context.ebp = (((uint32_t)stack) + KERNEL_STACK_SIZE);
    proc->context.esp = proc->context.ebp;
    proc->image.kernel_stack = stack;
}

static void __k_proc_process_idle() {
    while (1) {
        asm("pause");
    }
}

void k_proc_process_spawn(process_t* proc) {
    LOCK(proc_lock)

    EXTEND(processes, total_processes, sizeof(process_t*));
    proc->pid = total_processes;
    processes[total_processes - 1] = proc;
    k_info("Process spawned: %s (%d). Kernel stack: 0x%.8x", proc->name,
           proc->pid, proc->context.ebp);

    UNLOCK(proc_lock)
}

static void __k_proc_process_create_init() {
    process_t* proc = k_calloc(1, sizeof(process_t));

    strcpy(proc->name, "[kernel]");

    proc->context.esp = 0;
    proc->context.ebp = 0;
    proc->context.eip = 0;
    k_mem_paging_clone_pd(0, &proc->image.page_directory);

    k_proc_process_spawn(proc);
}

static process_t* __k_proc_process_create_idle() {
    process_t* proc = k_calloc(1, sizeof(process_t));

    strcpy(proc->name, "[idle]");

    __k_proc_process_create_kernel_stack(proc);

    proc->context.eip = (uint32_t)&__k_proc_process_idle;
    k_mem_paging_clone_pd(0, &proc->image.page_directory);

    k_proc_process_spawn(proc);
    return proc;
}

void k_proc_init_core() {
    current_core->idle_process = __k_proc_process_create_idle();
    current_core->current_process = current_core->idle_process;
}

void k_proc_process_init() {
    cli();

    total_processes = 0;
    next_process    = 0;

    __k_proc_process_create_init();

    k_proc_init_core();
    current_core->current_process = processes[0];

    sti();
}

extern __attribute__((returns_twice)) uint8_t
__k_proc_process_save(context_t* ctx);
extern __attribute__((noreturn)) void __k_proc_process_load(context_t* ctx);
extern __attribute__((noreturn)) void
__k_proc_process_enter_usermode(context_t* ctx, uint32_t userstack);

void k_proc_process_yield() {
    if (!total_processes) {
        return;
    }

    process_t* old_proc = k_proc_process_current();
    process_t* new_proc = k_proc_process_next();

    current_core->current_process = new_proc;

    if (!__k_proc_process_check_stack(new_proc)) {
        char buffer[256];
        sprintf(buffer,
                "Kernel stack smashing detected. \r\n EBP = 0x%.8x ESP = "
                "0x%.8x (0x%.8x) in %s (%d)",
                new_proc->context.ebp, new_proc->context.esp,
                new_proc->image.kernel_stack[0], new_proc->name, new_proc->pid);
        k_panic(buffer, 0);
    }

    if (__k_proc_process_save(&old_proc->context)) {
        return; // Just returned from switch, do nothing and let it return
    }

    uint8_t jump = new_proc->state == PROCESS_STATE_PL_CHANGE_REQUIRED;
    if (new_proc->state == PROCESS_STATE_STARTING || jump) {
        new_proc->state = PROCESS_STATE_STARTED;
    }

    k_mem_gdt_set_directory(new_proc->image.page_directory);
    k_mem_gdt_set_stack((uint32_t)new_proc->image.kernel_stack +
                        KERNEL_STACK_SIZE);

    k_mem_paging_set_pd(new_proc->image.page_directory, 1, 0);

    if (jump) {
        __k_proc_process_enter_usermode(&new_proc->context,
                                        USER_STACK_START + USER_STACK_SIZE);
    } else {
        __k_proc_process_load(&new_proc->context);
    }
}

uint32_t k_proc_process_exec(const char* path, int argc UNUSED,
                             char** argv UNUSED) {
    if (!total_processes) {
        return 0;
    }

    fs_node_t* node = k_fs_vfs_open(path);
    if (!node) {
        return 0;
    }

    uint8_t* buffer = k_malloc(node->size);
    k_fs_vfs_read(node, 0, node->size, buffer);

    process_t* proc = k_calloc(1, sizeof(process_t));
    strcpy(proc->name, node->name);
    k_fs_vfs_close(node);

    cli();

    uint32_t prev = k_mem_paging_get_pd(1);

    k_mem_paging_set_pd(0, 1, 0);
    k_mem_paging_clone_pd(0, &proc->image.page_directory);
    k_mem_paging_set_pd(proc->image.page_directory, 1, 0);

    uint32_t entry;
    if ((entry = k_mod_elf_load_exec(buffer))) {
        k_free(buffer);
        proc->context.eip = entry;

        __k_proc_process_create_kernel_stack(proc);
        k_mem_paging_map_region(USER_STACK_START, 0, USER_STACK_SIZE / 0x1000,
                                0x7, 0);

        k_proc_process_spawn(proc);
        proc->state = PROCESS_STATE_PL_CHANGE_REQUIRED;
    } else {
        k_free(buffer);
        k_free(proc);
    }

    k_mem_paging_set_pd(prev, 1, 0);

    sti();

    return proc->pid;
}

process_t* k_proc_process_current() {
    return current_core->current_process;
}

process_t* k_proc_process_next() {
    LOCK(proc_lock)

    if(!total_processes){
        k_panic("k_proc_process_next(): no idle task?", 0); //TODO idle task should not be on queue, it should just exists in current core
    }

    process_t* process = processes[next_process];
    // k_info("Next: %s", process->name);

    next_process++;
    if(next_process >= total_processes){
        next_process = 0;
    }

    UNLOCK(proc_lock)
    return process;
}

extern void __attribute__((noreturn)) __k_proc_process_fork_return();

uint32_t k_proc_process_fork() {
    if (!total_processes) {
        return 0;
    }

    process_t* src = current_core->current_process;
    process_t* new = k_malloc(sizeof(process_t));

    new->state = PROCESS_STATE_STARTING;

    memcpy(new, src, sizeof(process_t));

    uint32_t prev = k_mem_paging_get_pd(1);
    k_mem_paging_set_pd(new->image.page_directory, 1, 0);
    k_mem_paging_clone_pd(0, &new->image.page_directory);
    k_mem_paging_set_pd(prev, 1, 0);

    __k_proc_process_create_kernel_stack(new);

    new->syscall_state.eax = 0;

    PUSH(new->context.esp, interrupt_context_t, new->syscall_state)

    new->context.eip = (uint32_t)&__k_proc_process_fork_return;

    k_proc_process_spawn(new);

    return new->pid;
}
