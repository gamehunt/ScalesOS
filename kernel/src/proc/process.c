#include "dev/fpu.h"
#include "dev/timer.h"
#include "fs/vfs.h"
#include "int/isr.h"
#include "int/syscall.h"
#include "kernel.h"
#include "mem/gdt.h"
#include "mem/heap.h"
#include "mem/mmap.h"
#include "mem/paging.h"
#include "mem/memory.h"
#include "proc/spinlock.h"
#include "signal.h"
#include "util/asm_wrappers.h"
#include "util/log.h"
#include "util/panic.h"
#include "types/list.h"
#include "util/types/stack.h"
#include "types/tree.h"
#include "scales/sched.h"
#include "sched.h"
#include <proc/process.h>
#include <proc/smp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define GUARD_MAGIC 0xBEDAABED

#define SIG_IGNORE     0
#define SIG_TERMINATE  1
#define SIG_STOP       2

static uint8_t sig_defaults[] = {
	SIG_IGNORE,
	[SIGSTOP] = SIGSTOP,
	[SIGINT]  = SIG_TERMINATE,
	[SIGQUIT] = SIG_TERMINATE,
	[SIGABRT] = SIG_TERMINATE,
	[SIGKILL] = SIG_TERMINATE,
	[SIGUSR1] = SIG_TERMINATE,
	[SIGUSR2] = SIG_TERMINATE,
	[SIGSEGV] = SIG_TERMINATE,
	[SIGCHLD] = SIG_IGNORE,
};

static tree_t* 		process_tree = 0;
static list_t* 		process_list = 0;
static list_t* 		ready_queue  = 0;
static wait_node_t* sleep_queue  = 0;

static spinlock_t   sleep_lock   = 0;

static uint8_t __k_proc_process_check_stack(process_t* proc) {
    return proc->image.kernel_stack_base[0] == GUARD_MAGIC;
}

static void __k_proc_process_create_kernel_stack(process_t* proc) {
    uint32_t* stack = (uint32_t*) k_valloc(KERNEL_STACK_SIZE + sizeof(uint32_t), 4);
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

static void __k_proc_process_remove_from_sleep_queue(struct wait_node* target) {
	if(!target) {
		return;
	}
	wait_node_t* node 	   = sleep_queue;
	wait_node_t* prev_node = node;
	wait_node_t* next_node = NULL;
	int f = 0;
	while(node) {
		next_node = node->next;
		if(node == target) {
			f = 1;
			break;
		}
		prev_node = node;
		node      = next_node;
	}
	if(!f) {
		return;
	}
	if(node == sleep_queue) {
		sleep_queue = next_node;
	} else {
		prev_node->next = next_node;
	}
}

void k_proc_process_mark_ready(process_t* process) {
    LOCK(ready_queue->lock)
	if(k_proc_process_has_locks(process)) {
		process->flags |= PROCESS_FLAG_INTERRUPTED;
	} 
	process->state = PROCESS_STATE_RUNNING;
	k_proc_process_invalidate_locks(process);
    list_push_back(ready_queue, process);
    UNLOCK(ready_queue->lock)
}

static pid_t __k_proc_process_allocate_pid() {
	static pid_t pid = 2;
	return pid++;
}

static void __k_proc_process_spawn(process_t* proc, process_t* parent) {
    LOCK(process_list->lock)

    list_push_back(process_list, proc);

	proc->node = tree_create_node(proc);

	if(parent) {
		proc->uid      = parent->uid;
		proc->group_id = parent->pid;
		tree_insert_node(process_tree, proc->node, parent->node);
	}

    k_debug("Process spawned: %s (%d). Kernel stack: 0x%.8x", proc->name,
           proc->pid, proc->image.kernel_stack);

    UNLOCK(process_list->lock)
}

static block_node_t* __k_proc_process_init_block_node(process_t* proc) {
	proc->block_node = k_malloc(sizeof(block_node_t));
	proc->block_node->data.process = proc;
	proc->block_node->owners       = list_create();
	return proc->block_node;
}

static process_t* __k_proc_process_create_init() {
    process_t* proc = k_calloc(1, sizeof(process_t));

    strcpy(proc->name, "[init]");

	proc->pid = 1;
	proc->state = PROCESS_STATE_STARTING;

    __k_proc_process_create_kernel_stack(proc);

    proc->context.esp = 0;
    proc->context.ebp = 0;
    proc->context.eip = 0;
    proc->image.page_directory = k_mem_paging_clone_page_directory(NULL, NULL);
	proc->image.mmap_info.mmap_blocks = list_create();
	proc->context.fp_regs = k_valloc(512, 16);
	memset(proc->context.fp_regs, 0, 512);

	proc->wait_queue = list_create();

	__k_proc_process_init_block_node(proc);

	proc->wd_node = k_fs_vfs_open("/", O_RDONLY);
	strncpy(proc->wd, "/", 255);

    __k_proc_process_spawn(proc, 0);

    return proc;
}

static process_t* __k_proc_process_create_idle() {
    process_t* proc = k_calloc(1, sizeof(process_t));

    strcpy(proc->name, "[idle]");

	proc->pid = -1;

    __k_proc_process_create_kernel_stack(proc);

    proc->context.eip = (uint32_t)&__k_proc_process_idle;
	proc->context.esp = proc->image.kernel_stack;
	proc->context.ebp = proc->image.kernel_stack;
    proc->image.page_directory = k_mem_paging_clone_page_directory(NULL, NULL);

    return proc;
}

void k_proc_process_init_core() {
    current_core->idle_process    = __k_proc_process_create_idle();
    current_core->current_process = current_core->idle_process;
}

static void __k_proc_process_timer_callback(interrupt_context_t*);

void k_proc_process_init() {
	cli();

	process_tree       = tree_create();
    process_list       = list_create();
    ready_queue        = list_create();

    current_core->idle_process    = __k_proc_process_create_idle();
    current_core->current_process = __k_proc_process_create_init();

	tree_set_root(process_tree, current_core->current_process->node);

	k_dev_timer_add_callback(__k_proc_process_timer_callback);

	sti();
}

extern __attribute__((returns_twice)) uint8_t __k_proc_process_save(context_t* ctx);
extern __attribute__((noreturn)) void __k_proc_process_load(context_t* ctx);
extern __attribute__((noreturn)) void __k_proc_process_enter_usermode(uint32_t entry, uint32_t userstack);

static void __k_proc_process_update_stats() {
	uint64_t current_stamp = k_dev_timer_read_tsc();
	process_t* proc 	   = k_proc_process_current();

	if(proc->last_entrance && proc->last_entrance < current_stamp) {
		proc->time_total += current_stamp - proc->last_entrance;
	}
	proc->last_entrance = 0;

	if(proc->last_sys && proc->last_sys < current_stamp) {
		proc->time_system += current_stamp - proc->last_sys;
	}
	proc->last_sys = 0;
}

void k_proc_process_switch() {
    process_t* new_proc;

	do {
		new_proc = k_proc_process_next();
	}while(!PROCESS_IS_SCHEDULABLE(new_proc));

    current_core->current_process = new_proc;

	if(!PROCESS_IS_SCHEDULABLE(current_core->current_process)) {
        char buffer[256];
        sprintf(buffer, "Invalid process scheduled: %s (%d) in state %d!", new_proc->name, new_proc->pid, new_proc->state);
		k_panic(buffer, NULL);
	}

	if(current_core->current_process->state != PROCESS_STATE_RUNNING) {
		current_core->current_process->state = PROCESS_STATE_RUNNING;
	}

	__k_proc_process_update_stats();

	current_core->current_process->last_entrance = k_dev_timer_read_tsc();
	current_core->current_process->last_sys = current_core->current_process->last_entrance;

    if (!__k_proc_process_check_stack(new_proc)) {
        char buffer[256];
        sprintf(buffer,
                "Kernel stack smashing detected. \r\n EBP = 0x%.8x ESP = "
                "0x%.8x (0x%.8x) in %s (%d)",
                new_proc->context.ebp, new_proc->context.esp,
                new_proc->image.kernel_stack_base[0], new_proc->name, new_proc->pid);
        k_panic(buffer, 0);
    }

    k_mem_gdt_set_stack((uint32_t) new_proc->image.kernel_stack);
	k_mem_paging_set_page_directory(new_proc->image.page_directory, 0);

	__k_proc_process_load(&new_proc->context);
}

void k_proc_process_yield() {
    if (!process_list || !process_list->size) {
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

	k_dev_fpu_save(old_proc);

    if (__k_proc_process_save(&old_proc->context)) {
		k_dev_fpu_restore(old_proc);
        return; // Just returned from switch, do nothing and let it return
    }

	if(PROCESS_IS_SCHEDULABLE(old_proc)) {
    	k_proc_process_mark_ready(old_proc);
	}

    k_proc_process_switch(); 
}


process_t* k_proc_process_current() {
    return current_core->current_process;
}

list_t* k_proc_process_list() {
	return process_list;
}

process_t* k_proc_process_next() {
    process_t* process = list_pop_front(ready_queue);
    if(!process){
        return current_core->idle_process;
    }
    return process;
}

extern void __attribute__((noreturn)) __k_proc_process_fork_return();

uint32_t k_proc_process_fork() {
    if (!process_list || !process_list->size) {
        return 0;
    }

    process_t* src = (process_t*) current_core->current_process;
    process_t* new = k_malloc(sizeof(process_t));

    memcpy(new, src, sizeof(process_t));

	new->pid   = __k_proc_process_allocate_pid();
    new->state = PROCESS_STATE_STARTING;

	new->signals_ignored = 0;
	new->signal_queue    = 0;
	memset(new->signals, 0, sizeof(new->signals));

	if(new->wait_node) {
		new->wait_node = NULL;
	}
	if(new->timeout_node) {
		new->timeout_node = NULL;
	}
	new->wait_queue = list_create();

	__k_proc_process_init_block_node(new);

    new->image.page_directory = k_mem_paging_clone_page_directory(src->image.page_directory, NULL);
	new->image.mmap_info.mmap_blocks = list_create();

	for(size_t i = 0; i < src->image.mmap_info.mmap_blocks->size; i++) {
		mmap_block_t* src_block = src->image.mmap_info.mmap_blocks->data[i];
		mmap_block_t* block = k_malloc(sizeof(mmap_block_t));
		memcpy(block, src_block, sizeof(mmap_block_t));
		list_push_back(new->image.mmap_info.mmap_blocks, block);
	}

    __k_proc_process_create_kernel_stack(new);

    new->syscall_state.eax = 0;

    PUSH(new->image.kernel_stack, interrupt_context_t, new->syscall_state)

	new->context.esp = new->image.kernel_stack;
	new->context.ebp = new->image.kernel_stack;
    new->context.eip = (uint32_t)&__k_proc_process_fork_return;
	new->context.fp_regs = k_valloc(512, 16);
	memcpy(new->context.fp_regs, src->context.fp_regs, 512);

	new->fds.nodes = k_calloc(src->fds.size, sizeof(fd_t*));
	for(uint32_t i = 0; i < src->fds.size; i++) {
		fd_t* original_node = src->fds.nodes[i];
		if(!original_node) {
			new->fds.nodes[i] = 0;
		} else {
			new->fds.nodes[i] = k_malloc(sizeof(fd_t));
			memcpy(new->fds.nodes[i], original_node, sizeof(fd_t));
			new->fds.nodes[i]->node->links++;
			new->fds.nodes[i]->links = 1;
		}
	}

    __k_proc_process_spawn(new, src);
    k_proc_process_mark_ready(new);

    return new->pid;
}

uint32_t k_proc_process_clone(struct clone_args* args) {
	if (!process_list || !process_list->size) {
        return 0;
    }

    process_t* src = (process_t*) current_core->current_process;
    process_t* new = k_malloc(sizeof(process_t));

    memcpy(new, src, sizeof(process_t));

	new->pid   = __k_proc_process_allocate_pid();
    new->state = PROCESS_STATE_STARTING;

	new->signals_ignored = 0;
	new->signal_queue    = 0;
	memset(new->signals, 0, sizeof(new->signals));

	if(new->wait_node) {
		new->wait_node = NULL;
	}
	if(new->timeout_node) {
		new->timeout_node = NULL;
	}
	new->wait_queue = list_create();

	__k_proc_process_init_block_node(new);

	if(!(args->flags & CLONE_VM)) {
    	new->image.page_directory = k_mem_paging_clone_page_directory(src->image.page_directory, NULL);
		new->image.mmap_info.mmap_blocks = list_create();

		for(size_t i = 0; i < src->image.mmap_info.mmap_blocks->size; i++) {
			mmap_block_t* src_block = src->image.mmap_info.mmap_blocks->data[i];
			mmap_block_t* block = k_malloc(sizeof(mmap_block_t));
			memcpy(block, src_block, sizeof(mmap_block_t));
			list_push_back(new->image.mmap_info.mmap_blocks, block);
		}
	}

    __k_proc_process_create_kernel_stack(new);

    new->syscall_state.eax = 0;

    PUSH(new->image.kernel_stack, interrupt_context_t, new->syscall_state)

	new->context.esp = new->image.kernel_stack;
	new->context.ebp = new->image.kernel_stack;
    new->context.eip = (uint32_t)&__k_proc_process_fork_return;
	new->context.fp_regs = k_valloc(512, 16);
	memcpy(new->context.fp_regs, src->context.fp_regs, 512);

	if(args->stack) {
		void* stack = (void*) args->stack + args->stack_size;

		if(args->entry) {
			PUSH(stack, void*, args->arg);			
			PUSH(stack, void*, args->entry);			
		}

		new->syscall_state.esp = (uint32_t) stack;
		new->syscall_state.ebp = (uint32_t) stack;
		new->image.user_stack  = (uint32_t) stack;
	}

	if(!(args->flags & CLONE_FILES)) {
		new->fds.nodes = k_calloc(src->fds.size, sizeof(fd_t*));
		for(uint32_t i = 0; i < src->fds.size; i++) {
			fd_t* original_node = src->fds.nodes[i];
			if(!original_node) {
				new->fds.nodes[i] = 0;
			} else {
				new->fds.nodes[i] = k_malloc(sizeof(fd_t));
				memcpy(new->fds.nodes[i], original_node, sizeof(fd_t));
				new->fds.nodes[i]->node->links++;
				new->fds.nodes[i]->links = 1;
			}
		}
	}

    __k_proc_process_spawn(new, src);
    k_proc_process_mark_ready(new);

    return new->pid;

}

uint32_t k_proc_process_open_node(process_t* process, fs_node_t* node){
	LOCK(process->fds.lock);

	fd_t* fdt   = k_malloc(sizeof(fd_t));
	fdt->node   = node;
	fdt->offset = 0;
	fdt->links  = 1;
	
	if(process->fds.size == process->fds.amount) {
		uint32_t fd = process->fds.size;
		EXTEND(process->fds.nodes, process->fds.size, sizeof(fd_t*));
		process->fds.nodes[fd] = fdt;
		process->fds.amount++;
		UNLOCK(process->fds.lock);
		return fd;
	} else{
		for(uint32_t i = 0; i < process->fds.size; i++) {
			if(!process->fds.nodes[i]){
				process->fds.nodes[i] = fdt;
				process->fds.amount++;
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
	
	k_fs_vfs_close(process->fds.nodes[fd]->node);
	process->fds.nodes[fd]->links--;
	if(process->fds.nodes[fd]->links <= 0) {
		k_free(process->fds.nodes[fd]);
	}

	process->fds.nodes[fd] = 0;
	UNLOCK(process->fds.lock);
}

void  k_proc_process_own_block(process_t* process, list_t* queue) {
	list_push_back(queue, process->block_node);
	list_push_back(process->block_node->owners, queue);
}

void k_proc_process_release_block(process_t* process) {
	block_node_t* node = process->block_node;
	while(node->owners->size) {
		list_t* l = list_pop_back(node->owners);
		list_delete_element(l, node);
	}
}

uint8_t k_proc_process_sleep_on_queue(process_t* process, list_t* queue) {
	k_proc_process_own_block(process, queue);
	process->flags &= ~PROCESS_FLAG_INTERRUPTED;
	process->state = PROCESS_STATE_SLEEPING;
	if(process == k_proc_process_current()) {
		k_proc_process_yield();
	}
	return (process->flags & PROCESS_FLAG_INTERRUPTED);
}

uint8_t k_proc_process_sleep_and_unlock(process_t* process, list_t* queue, spinlock_t* lock) {
	k_proc_process_own_block(process, queue);
	process->flags &= ~PROCESS_FLAG_INTERRUPTED;
	process->state = PROCESS_STATE_SLEEPING;
	UNLOCK(*lock)
	if(process == k_proc_process_current()) {
		k_proc_process_yield();
	}
	return (process->flags & PROCESS_FLAG_INTERRUPTED);
}

void k_proc_process_wakeup_queue_single(list_t* queue) {
	while(queue->size > 0) {
		block_node_t* node = list_pop_back(queue);
		list_delete_element(node->owners, queue);
		process_t* process = node->data.process;
		if(process->state != PROCESS_STATE_FINISHED && !k_proc_process_is_ready(process)) {
			process->state = PROCESS_STATE_RUNNING;
			k_proc_process_mark_ready(process);
			return;
		}
	}
}

void k_proc_process_wakeup_queue_single_select(list_t* queue, fs_node_t* fsnode, uint8_t event) {
	while(queue->size > 0) {
		block_node_t* node = list_pop_back(queue);
		process_t* process = node->data.process;
		list_delete_element(node->owners, queue);
		k_proc_process_release_block(process);
		if(process->state != PROCESS_STATE_FINISHED && !k_proc_process_is_ready(process)) {
			process->state = PROCESS_STATE_RUNNING;
			process->select_wait_node  = fsnode;
			process->select_wait_event = event;
			if(process->timeout_node) {
				__k_proc_process_remove_from_sleep_queue(process->timeout_node);
				k_free(process->timeout_node);
				process->timeout_node = NULL;
			}
			k_proc_process_mark_ready(process);
			return;
		}
	}
}

void k_proc_process_wakeup_queue(list_t* queue) {
	while(queue->size > 0) {
		k_proc_process_wakeup_queue_single(queue);
	}
}

void k_proc_process_wakeup_queue_select(list_t* queue, fs_node_t* node, uint8_t event) {
	while(queue->size > 0) {
		k_proc_process_wakeup_queue_single_select(queue, node, event);
	}
}

uint8_t k_proc_process_has_locks(process_t* process) {
	return process->block_node->owners->size > 0 || process->timeout_node || process->wait_node;
}
void k_proc_process_invalidate_locks(process_t* process) {
	while(process->block_node->owners->size) {
		list_t* owner = list_pop_back(process->block_node->owners);
		LOCK(owner->lock)
		list_delete_element(owner, process->block_node);
		UNLOCK(owner->lock)
	}
	if(process->wait_node) {
		__k_proc_process_remove_from_sleep_queue(process->wait_node);	
		k_free(process->wait_node);
		process->wait_node = NULL;
	}

	if(process->timeout_node) {
		__k_proc_process_remove_from_sleep_queue(process->timeout_node);	
		k_free(process->timeout_node);
		process->timeout_node = NULL;
	}
}

void k_proc_process_exit(process_t* process, int code) {
	k_debug("Process %s (%d) exited with code %d", process->name, process->pid, code);

	process->status = code;

	fd_list_t* list = &process->fds;
	for(uint32_t i = 0; i < list->size; i++){
		if(list->nodes[i]) {
			k_proc_process_close_fd(process, i);
		}
	}
	k_free(list->nodes);

	process->state = PROCESS_STATE_FINISHED;

	LOCK(process_tree->root->children->lock);
	for(uint32_t i = 0; i < process->node->children->size; i++) {
		tree_node_t* node = (tree_node_t*) process->node->children->data[i];
		node->parent = process_tree->root;
		list_push_back(process_tree->root->children, node);
	}
	UNLOCK(process_tree->root->children->lock);

	while(process->image.mmap_info.mmap_blocks->size > 0) {
		mmap_block_t* block = list_pop_back(process->image.mmap_info.mmap_blocks);
		k_mem_mmap_free_block(&process->image.mmap_info, block);
	}

	if(process->node->parent) {
		process_t* parent = process->node->parent->value;
		if(parent) {
			k_proc_process_wakeup_queue(parent->wait_queue);
			k_proc_process_send_signal(parent, SIGCHLD);
		}
	}

	k_proc_process_invalidate_locks(process);

	list_free(process->block_node->owners);
	k_free(process->block_node);

	k_proc_process_switch();
}

void* k_proc_process_grow_heap(process_t* process, int32_t size){
	if(size <= 0) {
		return NULL;
	}

	pde_t* old_pd = k_mem_paging_get_page_directory(NULL);
	k_mem_paging_set_page_directory(process->image.page_directory, 0);

	uint32_t mapping_start = process->image.heap + process->image.heap_size;

	k_mem_paging_map_region(mapping_start, 0, size, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER, 0);
	process->image.heap_size += size * 0x1000;

	k_mem_paging_set_page_directory(old_pd, 0);

	return (void*) (mapping_start);
}

uint8_t __k_proc_process_waitpid_can_pick(process_t* process, process_t* parent, int pid) {
	if(pid < -1) {
		return process->pid == -pid;
	} else if(pid == 0) {
		return process->group_id == parent->group_id; 
	} else if(pid > 0) {
		return process->pid == pid;
	} else {
		return 1;
	}
}

pid_t k_proc_process_waitpid(process_t* process, int pid, int* status, int options) {
	if(options > PROCESS_WAITPID_WUNTRACED) {
		return -EINVAL;
	}

	do {
		process_t* child = 0;
		uint8_t was = 0;
		for(uint32_t i = 0; i < process->node->children->size; i++) {
			process_t* candidate = ((tree_node_t*)process->node->children->data[i])->value;
			if(__k_proc_process_waitpid_can_pick(candidate, process, pid)) {
				was = 1;
				if(candidate->state == PROCESS_STATE_FINISHED) {
					child = candidate;
					break;
				}	
			}
		}

		if(!was) {
			return -ECHILD;
		}
		
		if(child) {
			if(status && IS_VALID_PTR((uint32_t) status)){
				*status = child->status;
			}
			k_proc_process_destroy(child);
			return child->pid;
		} else if(!(options & PROCESS_WAITPID_WNOHANG)) {
			k_proc_process_sleep_on_queue(process, process->wait_queue);
		} else{
			return -EINTR;
		}
	} while(1);
}


void k_proc_process_destroy(process_t* process) {
	list_free(process->image.mmap_info.mmap_blocks);
	list_delete_element(process_list, process);
	tree_remove_node(process_tree, process->node);
	tree_free_node(process->node);
	list_free(process->wait_queue);
    k_mem_paging_release_directory(process->image.page_directory);
	k_vfree(process->context.fp_regs);
	k_vfree(process->image.kernel_stack_base);
	k_free(process);
}

static void __k_proc_process_timer_callback(interrupt_context_t* regs UNUSED) {
	k_proc_process_update_timings();
}

#define MICROSECONDS_PER_SECOND 1000000
#define TICKS_SECONDS(x)      (x / MICROSECONDS_PER_SECOND)
#define TICKS_MICROSECONDS(x) (x % MICROSECONDS_PER_SECOND)

static void __k_proc_process_wakeup_timing(uint64_t seconds, uint64_t microseconds) {
	LOCK(sleep_lock)
	wait_node_t* node = sleep_queue; 
	while(node && (node->seconds < seconds || (node->seconds == seconds && node->microseconds <= microseconds))) {
		sleep_queue = node->next;
		process_t* process = node->data.process;
		if(process->state != PROCESS_STATE_FINISHED) {
			if(process->timeout_node == node) {
				k_proc_process_release_block(process);
				process->select_wait_node             = 0;
				process->select_wait_event            = 0;
				process->timeout_node                 = NULL;
			} else {
				node->data.process->wait_node = NULL;
			}
			k_proc_process_mark_ready(process);
		}
		k_free(node);
		node = sleep_queue;
	}
	UNLOCK(sleep_lock)
}

static uint64_t __k_proc_get_tsc_ticks() {
	uint64_t tsc_ticks = k_dev_timer_read_tsc();
	uint64_t speed     = k_dev_timer_get_core_speed();
	uint64_t base      = k_dev_timer_tsc_base();

	uint64_t ticks     = (tsc_ticks / speed) - base; 

	return ticks;
}

void k_proc_process_timeout(process_t* process, uint64_t seconds, uint64_t microseconds) {
	uint64_t ticks   = __k_proc_get_tsc_ticks();

	uint64_t cur_seconds      = TICKS_SECONDS(ticks);
	uint64_t cur_microseconds = TICKS_MICROSECONDS(ticks);

	cur_seconds      += seconds;
	cur_microseconds += microseconds;

	if(cur_microseconds >= MICROSECONDS_PER_SECOND) {
		cur_seconds      += cur_microseconds / MICROSECONDS_PER_SECOND;
		cur_microseconds %= MICROSECONDS_PER_SECOND;
	}

	wait_node_t* new_node   = k_calloc(1, sizeof(wait_node_t));
	new_node->data.process 	= process;
	new_node->seconds      	= cur_seconds;
	new_node->microseconds 	= cur_microseconds;
	process->timeout_node  	= new_node;

	LOCK(sleep_lock)
	if(!sleep_queue) {
		sleep_queue = new_node;
		sleep_queue->next = 0;
	} else {
		wait_node_t* node 	   = sleep_queue;
		wait_node_t* prev_node = node;
		while(node && (node->seconds > cur_seconds || (node->seconds == cur_seconds && node->microseconds > cur_microseconds))) {
			prev_node = node;
			node      = node->next;
		}
		prev_node->next = new_node;
		new_node->next  = node;
	}
	UNLOCK(sleep_lock)
}

uint32_t k_proc_process_sleep(process_t* process, uint64_t microseconds) {
	uint64_t seconds = TICKS_SECONDS(microseconds);
	microseconds     = TICKS_MICROSECONDS(microseconds);

	uint64_t ticks   = __k_proc_get_tsc_ticks();

	uint64_t cur_seconds      = TICKS_SECONDS(ticks);
	uint64_t cur_microseconds = TICKS_MICROSECONDS(ticks);

	cur_seconds      += seconds;
	cur_microseconds += microseconds;

	if(cur_microseconds >= MICROSECONDS_PER_SECOND) {
		cur_seconds      += cur_microseconds / MICROSECONDS_PER_SECOND;
		cur_microseconds %= MICROSECONDS_PER_SECOND;
	}

	wait_node_t* new_node  = k_calloc(1, sizeof(wait_node_t));
	new_node->data.process 	= process;
	new_node->seconds      	= cur_seconds;
	new_node->microseconds 	= cur_microseconds;
	process->wait_node 	   	= new_node;

	LOCK(sleep_lock)
	if(!sleep_queue) {
		sleep_queue = new_node;
		sleep_queue->next = 0;
	} else {
		wait_node_t* node 	   = sleep_queue;
		wait_node_t* prev_node = node;
		while(node && (node->seconds > cur_seconds || (node->seconds == cur_seconds && node->microseconds > cur_microseconds))) {
			prev_node = node;
			node      = node->next;
		}
		prev_node->next = new_node;
		new_node->next  = node;
	}
	UNLOCK(sleep_lock)

	process->state = PROCESS_STATE_SLEEPING;
	k_proc_process_yield();

	ticks   = __k_proc_get_tsc_ticks();

	uint64_t new_seconds      = TICKS_SECONDS(ticks);
	uint64_t new_microseconds = TICKS_MICROSECONDS(ticks);

	if(cur_seconds > new_seconds || (cur_seconds == new_microseconds && cur_microseconds > new_microseconds)) {
		return 1;
	} else {
		return 0;
	}
}

void k_proc_process_update_timings() {
	uint64_t ticks = __k_proc_get_tsc_ticks();

	uint64_t seconds      = TICKS_SECONDS(ticks);
	uint64_t microseconds = TICKS_MICROSECONDS(ticks);

	__k_proc_process_wakeup_timing(seconds, microseconds);
}


uint8_t k_proc_process_is_ready(process_t* process) {
	return PROCESS_IS_SCHEDULABLE(process);
}

void k_proc_process_wakeup_on_signal(process_t* process) {
	if(process != current_core->current_process && !k_proc_process_is_ready(process)) {
		k_proc_process_mark_ready(process);
	}
}

void k_proc_process_send_signal(process_t* process, int sig) {
	if(sig < 0 || sig >= MAX_SIGNAL) {
		return;
	}

	process->signal_queue |= (1 << sig);

	k_proc_process_wakeup_on_signal(process);
}

extern interrupt_context_t* __k_int_syscall_dispatcher(interrupt_context_t*);

static void __k_proc_process_try_restart_syscalls(process_t* process, int sig, interrupt_context_t* ctx) {
	if(sig < 0 || sig >= MAX_SYSCALL) {
		return;
	}

	if(process->interrupted_syscall && (int32_t) ctx->eax == -ERESTARTSYS) {
		if(process->signals[sig].flags & SA_RESTART) {
			ctx->eax = process->interrupted_syscall;
			process->interrupted_syscall = 0;
			__k_int_syscall_dispatcher(ctx);
		} else {
			process->interrupted_syscall = 0;
			ctx->eax = -EINTR;
		}
	}
}

static int __k_proc_process_handle_signal(process_t* process, int sig, interrupt_context_t* ctx) {
	if(sig < 0 || sig >= MAX_SIGNAL) {
		return 1;
	}

	if(IS_SIGNAL_IGNORED(process, sig)) {
		return 1;
	}

	signal_t cfg = process->signals[sig];

	if(!cfg.handler) {
		uint8_t def = sig_defaults[sig];
		if(def == SIG_TERMINATE) {
			k_proc_process_exit(process, ((128 + sig) << 4) | sig);
		} else if (def == SIG_STOP) {
			process->state = PROCESS_STATE_SLEEPING;

			process_t* parent = process->node->parent->value;

			if(parent) {
				k_proc_process_wakeup_queue(parent->wait_queue);
			}

			return 1;
		}
	} else {
		uint32_t esp = ctx->esp;

		PUSH(esp, interrupt_context_t, *ctx);
		PUSH(esp, int, sig);
		PUSH(esp, uintptr_t, 0xDEADBEEF);

		k_debug("Jumping to %d sig handler at %#.8x", sig, cfg.handler);

		__k_proc_process_enter_usermode((uint32_t) cfg.handler, esp);
		__builtin_unreachable();
	}

	return !process->signal_queue;
}

void k_proc_process_process_signals(process_t* process, interrupt_context_t* ctx) {
	if(!process->signal_queue) {
		return;
	}
	for(int i = 0; i < MAX_SIGNAL; i++) {
		uint64_t check = (1 << i);
		if(process->signal_queue & check) {
			process->signal_queue &= ~check;
			if(__k_proc_process_handle_signal(process, i, ctx)){
				return;
			}
		}
	}
}

process_t* k_proc_process_find_by_pid(pid_t pid) {
	for(uint32_t i = 0; i < process_list->size; i++) {
		process_t* process = process_list->data[i];
		if(process->pid == pid) {
			return process;
		}
	}
	return 0;
}

void k_proc_process_return_from_signal(interrupt_context_t* ctx) {
	process_t* proc = k_proc_process_current();

	int sig;
	POP(ctx->esp, int, sig);

	interrupt_context_t out;
	POP(ctx->esp, interrupt_context_t, out);

	ctx->eip = out.eip;

	ctx->esp = out.esp;
	ctx->ebp = out.ebp;

	ctx->eax = out.eax;
	ctx->ebx = out.ebx;
	ctx->ecx = out.ecx;
	ctx->edx = out.edx;
	
	ctx->edi = out.edi;
	ctx->esi = out.esi;

	if(proc->signal_queue) {
		k_proc_process_process_signals(proc, ctx);
	}

	__k_proc_process_try_restart_syscalls(proc, sig, ctx);
}

extern void __k_proc_process_enter_tasklet(void);

pid_t k_proc_process_create_tasklet(const char* name, uintptr_t entry, void* data) {
	process_t* proc = k_calloc(1, sizeof(process_t));
	strcpy(proc->name, name);

	proc->pid = __k_proc_process_allocate_pid();
	proc->flags = PROCESS_FLAG_TASKLET;
	proc->state = PROCESS_STATE_STARTING;

	proc->image.page_directory = k_mem_paging_clone_root_page_directory(NULL);
	__k_proc_process_create_kernel_stack(proc);

	PUSH(proc->image.kernel_stack, void*, data);
	PUSH(proc->image.kernel_stack, uintptr_t, entry);

	proc->context.ebp = proc->image.kernel_stack;
	proc->context.esp = proc->image.kernel_stack;
	proc->context.eip = (uintptr_t) &__k_proc_process_enter_tasklet;
	proc->context.fp_regs = k_valloc(512, 16);
	memset(proc->context.fp_regs, 0, 512);

	proc->wait_queue 				  = list_create();
	proc->image.mmap_info.mmap_blocks = list_create();
	
	__k_proc_process_init_block_node(proc);

	__k_proc_process_spawn(proc, process_tree->root->value);
	k_proc_process_mark_ready(proc);

	return proc->pid;
}

void k_proc_process_prepare_exec() {
	process_t* proc = k_proc_process_current();

	fd_list_t* list = &proc->fds;
	for(uint32_t i = 0; i < list->size; i++){
		if(list->nodes[i]) {
			fs_node_t* node = list->nodes[i]->node;
			if(node->mode & O_CLOEXEC) {
				k_proc_process_close_fd(proc, i);
			}
		}
	}
}

