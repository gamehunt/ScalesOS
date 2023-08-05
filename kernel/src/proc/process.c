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
#include "util/types/list.h"
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

static list_t* processes = 0;
static list_t* ready_processes = 0;

static uint8_t __k_proc_process_check_stack(process_t* proc) {
    if (proc->pid == 1) {
        return 1;
    }
    return proc->image.kernel_stack_base[0] == GUARD_MAGIC;
}

static void __k_proc_process_create_kernel_stack(process_t* proc) {
    uint32_t* stack = k_valloc(KERNEL_STACK_SIZE + sizeof(uint32_t), 4);
    memset(stack, 0, KERNEL_STACK_SIZE);
    stack[0] = GUARD_MAGIC;
    proc->image.kernel_stack = (uint32_t) stack + KERNEL_STACK_SIZE;
	proc->image.kernel_stack_base = stack;
}

static void __k_proc_process_idle() {
    while (1) {
        asm volatile(
            "sti\n"
            "hlt\n"
            "cli\n"
        );
    }
}

void k_proc_process_mark_ready(process_t* process) {
    LOCK(ready_processes->lock)
    list_push_back(ready_processes, process);
    UNLOCK(ready_processes->lock)
}

void __k_proc_process_spawn(process_t* proc) {
    LOCK(processes->lock)

    proc->pid = processes->size + 1;
    list_push_back(processes, proc);

    k_info("Process spawned: %s (%d). Kernel stack: 0x%.8x", proc->name,
           proc->pid, proc->image.kernel_stack);

    UNLOCK(processes->lock)
}

static process_t* __k_proc_process_create_init() {
    process_t* proc = k_calloc(1, sizeof(process_t));

    strcpy(proc->name, "[kernel]");

    __k_proc_process_create_kernel_stack(proc);

    proc->context.esp = 0;
    proc->context.ebp = 0;
    proc->context.eip = 0;
    k_mem_paging_clone_pd(0, &proc->image.page_directory);

    __k_proc_process_spawn(proc);

    return proc;
}

static process_t* __k_proc_process_create_idle() {
    process_t* proc = k_calloc(1, sizeof(process_t));

    strcpy(proc->name, "[idle]");

    __k_proc_process_create_kernel_stack(proc);

    proc->context.eip = (uint32_t)&__k_proc_process_idle;
    k_mem_paging_clone_pd(0, &proc->image.page_directory);

    return proc;
}

void k_proc_process_init_core() {
    current_core->idle_process    = __k_proc_process_create_idle();
    current_core->current_process = current_core->idle_process;
}

void k_proc_process_init() {
    processes       = list_create();
    ready_processes = list_create();

    current_core->idle_process    = __k_proc_process_create_idle();
    current_core->current_process = __k_proc_process_create_init();
}

extern __attribute__((returns_twice)) uint8_t __k_proc_process_save(context_t* ctx);
extern __attribute__((noreturn)) void __k_proc_process_load(context_t* ctx);
extern __attribute__((noreturn)) void __k_proc_process_enter_usermode(uint32_t entry, uint32_t userstack);

void k_proc_process_switch() {
    process_t* new_proc;

	do {
		new_proc = k_proc_process_next();
	}while(new_proc->state == PROCESS_STATE_FINISHED);

    current_core->current_process = new_proc;

    if (!__k_proc_process_check_stack(new_proc)) {
        char buffer[256];
        sprintf(buffer,
                "Kernel stack smashing detected. \r\n EBP = 0x%.8x ESP = "
                "0x%.8x (0x%.8x) in %s (%d)",
                new_proc->context.ebp, new_proc->context.esp,
                new_proc->image.kernel_stack_base[0], new_proc->name, new_proc->pid);
        k_panic(buffer, 0);
    }

    k_mem_gdt_set_directory(new_proc->image.page_directory);
    k_mem_gdt_set_stack((uint32_t) new_proc->image.kernel_stack);
    k_mem_paging_set_pd(new_proc->image.page_directory, 1, 0);

    __k_proc_process_load(&new_proc->context);
}

void k_proc_process_yield() {
    if (!processes || !processes->size) {
        return;
    }

    process_t* old_proc = k_proc_process_current();
    if(!old_proc){
        return;
    }

    if(old_proc == current_core->idle_process){
        k_proc_process_switch();
        return;
    }

    if (__k_proc_process_save(&old_proc->context)) {
        return; // Just returned from switch, do nothing and let it return
    }

    k_proc_process_mark_ready(old_proc);

    k_proc_process_switch(); 
}

void k_proc_process_exec(const char* path, int argc UNUSED, char** argv UNUSED) {
    if (!processes || !processes->size) {
        return;
    }

    fs_node_t* node = k_fs_vfs_open(path);
    if (!node) {
        return;
    }

    uint8_t* buffer = k_malloc(node->size);
    k_fs_vfs_read(node, 0, node->size, buffer);

    process_t* proc = k_proc_process_current(); 
    strcpy(proc->name, node->name);

    k_info("Executing: %s", proc->name);

    k_fs_vfs_close(node);

    uint32_t entry;
    if ((entry = k_mod_elf_load_exec(buffer))) {
        k_free(buffer);

        k_mem_paging_map_region(USER_STACK_START, 0, USER_STACK_SIZE / 0x1000, 0x7, 0);

        k_mem_gdt_set_stack((uint32_t) proc->image.kernel_stack);
        k_mem_gdt_set_directory(proc->image.page_directory);
        __k_proc_process_enter_usermode(entry, USER_STACK_START + USER_STACK_SIZE);
    } else {
        k_free(buffer);
    }
}

process_t* k_proc_process_current() {
    return current_core->current_process;
}

process_t* k_proc_process_next() {
    LOCK(ready_processes->lock)

    process_t* process = list_pop_front(ready_processes);
    if(!process){
        return current_core->idle_process;
    }

    UNLOCK(ready_processes->lock)
    return process;
}

extern void __attribute__((noreturn)) __k_proc_process_fork_return();

uint32_t k_proc_process_fork() {
    if (!processes || !processes->size) {
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

    PUSH(new->image.kernel_stack, interrupt_context_t, new->syscall_state)

	new->context.esp = new->image.kernel_stack;
	new->context.ebp = new->image.kernel_stack;
    new->context.eip = (uint32_t)&__k_proc_process_fork_return;

    __k_proc_process_spawn(new);
    k_proc_process_mark_ready(new);

    return new->pid;
}

uint32_t k_proc_process_open_node(process_t* process, fs_node_t* node){
	LOCK(process->fds.lock);
	if(process->fds.size == process->fds.amount) {
		uint32_t fd = process->fds.size;
		EXTEND(process->fds.nodes, process->fds.size, sizeof(fs_node_t));
		process->fds.nodes[fd] = node;
		process->fds.amount++;
		UNLOCK(process->fds.lock);
		return fd;
	} else{
		for(uint32_t i = 0; i < process->fds.size; i++) {
			if(!process->fds.nodes[i]){
				process->fds.nodes[i] = node;
				UNLOCK(process->fds.lock);
				return i;
			}
		}
	}
	__builtin_unreachable();
}

void k_proc_process_close_fd(process_t* process, uint32_t fd){
	LOCK(process->fds.lock);
	if(process->fds.size <= fd) {
		k_warn("Process %s (%d) tried to close invalid fd: %d", process->name, process->pid, fd);
		UNLOCK(process->fds.lock);
		return;
	}
	process->fds.amount--;
	k_fs_vfs_close(process->fds.nodes[fd]);
	process->fds.nodes[fd] = 0;
	UNLOCK(process->fds.lock);
}

void k_proc_process_exit(process_t* process, int code) {
	k_info("Process %s (%d) exited with code %d", process->name, process->pid, code);

	fd_list_t* list = &process->fds;
	for(uint32_t i = 0; i < list->size; i++){
		if(list->nodes[i]) {
			k_fs_vfs_close(list->nodes[i]);
		}
	}
	k_free(list->nodes);

	process->state = PROCESS_STATE_FINISHED;

	k_proc_process_switch();
}
