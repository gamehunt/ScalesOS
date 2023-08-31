#include "dev/timer.h"
#include "fs/vfs.h"
#include "int/isr.h"
#include "kernel.h"
#include "mem/gdt.h"
#include "mem/heap.h"
#include "mem/paging.h"
#include "mem/memory.h"
#include "mod/elf.h"
#include "proc/spinlock.h"
#include "signal.h"
#include "util/log.h"
#include "util/panic.h"
#include "util/types/list.h"
#include "util/types/stack.h"
#include "util/types/tree.h"
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
	[SIGKILL] = SIG_TERMINATE,
	[SIGSEGV] = SIG_TERMINATE
};

static tree_t* process_tree    = 0;
static list_t* process_list    = 0;
static list_t* ready_queue     = 0;
static wait_node_t* sleep_queue     = 0;

static uint8_t __k_proc_process_check_stack(process_t* proc) {
    if (proc->pid == 1) {
        return 1;
    }
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

void k_proc_process_mark_ready(process_t* process) {
    LOCK(ready_queue->lock)
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

    k_info("Process spawned: %s (%d). Kernel stack: 0x%.8x", proc->name,
           proc->pid, proc->image.kernel_stack);

    UNLOCK(process_list->lock)
}

static process_t* __k_proc_process_create_init() {
    process_t* proc = k_calloc(1, sizeof(process_t));

    strcpy(proc->name, "[init]");

	proc->pid = 1;

    __k_proc_process_create_kernel_stack(proc);

    proc->context.esp = 0;
    proc->context.ebp = 0;
    proc->context.eip = 0;
    k_mem_paging_clone_pd(0, &proc->image.page_directory);

	proc->wait_queue = list_create();

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
    k_mem_paging_clone_pd(0, &proc->image.page_directory);

    return proc;
}

void k_proc_process_init_core() {
    current_core->idle_process    = __k_proc_process_create_idle();
    current_core->current_process = current_core->idle_process;
}

static void __k_proc_process_timer_callback(interrupt_context_t*);

void k_proc_process_init() {
	process_tree       = tree_create();
    process_list       = list_create();
    ready_queue        = list_create();

    current_core->idle_process    = __k_proc_process_create_idle();
    current_core->current_process = __k_proc_process_create_init();

	tree_set_root(process_tree, current_core->current_process->node);

	k_dev_timer_add_callback(__k_proc_process_timer_callback);
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
	}while(new_proc->state == PROCESS_STATE_FINISHED);

    current_core->current_process = new_proc;

	__k_proc_process_update_stats();

	current_core->current_process->last_entrance = k_dev_timer_read_tsc();
	current_core->current_process->last_sys = current_core->current_process->last_entrance;

	if(current_core->current_process->state == PROCESS_STATE_STARTING) {
		current_core->current_process->state = PROCESS_STATE_RUNNING;
	}

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

    if (__k_proc_process_save(&old_proc->context)) {
        return; // Just returned from switch, do nothing and let it return
    }

	if(old_proc->state != PROCESS_STATE_SLEEPING && old_proc->state != PROCESS_STATE_FINISHED) {
    	k_proc_process_mark_ready(old_proc);
	}

    k_proc_process_switch(); 
}

int k_proc_process_exec(const char* path, char** argv, char** envp) {
    if (!process_list || !process_list->size) {
        return -1;
    }

    fs_node_t* node = k_fs_vfs_open(path, O_RDONLY);
    if (!node) {
        return -2;
    }

    uint8_t* buffer = k_malloc(node->size);
    uint32_t read = k_fs_vfs_read(node, 0, node->size, buffer);
    k_fs_vfs_close(node);
	if(read != node->size) {
		k_err("Failed to read whole file.");
		return -4;
	}


    process_t* proc = k_proc_process_current(); 
    strcpy(proc->name, node->name);

	uint32_t* prev = (uint32_t*) k_mem_paging_get_pd(0);
	k_mem_paging_set_pd(0, 0, 0);
	k_mem_paging_clone_pd(0, &proc->image.page_directory);
	k_mem_paging_set_pd(proc->image.page_directory, 1, 0);
	//TODO release old directory

    k_info("Executing: %s", proc->name);

	proc->image.heap       = USER_HEAP_START;
	proc->image.heap_size  = USER_HEAP_INITIAL_SIZE;
	proc->image.user_stack = USER_STACK_START + USER_STACK_SIZE;

    uint32_t entry;
    if ((entry = k_mod_elf_load_exec(buffer))) {
        k_free(buffer);

        k_mem_paging_map_region(USER_STACK_START, 0, USER_STACK_SIZE / 0x1000, 0x7, 0);
		k_mem_paging_map_region(USER_HEAP_START,  0, USER_HEAP_INITIAL_SIZE / 0x1000, 0x7, 0);
		
		int       envc       = 0;
		uintptr_t envp_stack = 0; 

		if(envp) {
			while(envp[envc]) {
				envc++;
			}

			if(envc) {
				char** ptrs = k_calloc(envc, sizeof(char*));
				int j = 0;
				for(int env = envc - 1; env >= 0; env--) {
					PUSH(proc->image.user_stack, char, '\0');
					for(int i = strlen(envp[env]) - 1; i >= 0; i--) {
						PUSH(proc->image.user_stack, char, envp[env][i]);
					}
					ptrs[j] = (char*) proc->image.user_stack;
					j++;
				}

				for(int i = 0; i < envc; i++) {
					PUSH(proc->image.user_stack, char*, ptrs[i]);
				}
				
				k_free(ptrs);

				char** _envp = (char**) proc->image.user_stack;
				envp_stack = (uintptr_t) _envp;
			} 
		}		

		int       argc       = 1;
		uintptr_t argv_stack = 0;

		if(!argv) {
			argv    = malloc(sizeof(char*) * 2);
			argv[0] = 0;
		} else {
			while(argv[argc - 1]) {
				argc++;
			}
			argv = realloc(argv, sizeof(char*) * (argc + 1));
			memmove(argv + 1, argv, sizeof(char*) * argc);
		}

		argv[0] = malloc(strlen(proc->name) + 1);
		strcpy(argv[0], proc->name);

		char** ptrs = k_calloc(argc, sizeof(char*)); 
		for(int i = argc - 1; i >= 0; i--) {
			PUSH(proc->image.user_stack, char, '\0');
			for(int j = strlen(argv[i]) - 1; j >= 0; j--) {
				PUSH(proc->image.user_stack, char, argv[i][j])		
			}
			ptrs[i] = (char*) proc->image.user_stack;
		}

		k_free(argv);
        
		for(int i = argc - 1; i >= 0; i--) {
			PUSH(proc->image.user_stack, char*, ptrs[i])
		}
        
		k_free(ptrs);

		char** _argv = (char**) proc->image.user_stack;
		argv_stack = (uintptr_t) _argv;

		while(proc->image.user_stack % 16) {
			PUSH(proc->image.user_stack, char, '\0');
		} 

		PUSH(proc->image.user_stack, uintptr_t, envp_stack);
		PUSH(proc->image.user_stack, int,       envc);
		PUSH(proc->image.user_stack, uintptr_t, argv_stack);
		PUSH(proc->image.user_stack, int,       argc);
		
		// uint32_t occupied_stack_size = USER_STACK_START + USER_STACK_SIZE - proc->image.user_stack;
		// for(uint32_t i = 0; i < occupied_stack_size; i += 4) {
		// 	k_debug("[0x%.8x] 0x%.8x", proc->image.user_stack + i, *((uint32_t*) (proc->image.user_stack + i)));
		// }
		
		proc->image.entry = entry;

        k_mem_gdt_set_stack((uint32_t) proc->image.kernel_stack);
        k_mem_gdt_set_directory(proc->image.page_directory);
        __k_proc_process_enter_usermode(entry, proc->image.user_stack);
    } else {
        k_free(buffer);
    }

	return -3;
}

process_t* k_proc_process_current() {
    return current_core->current_process;
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

    process_t* src = current_core->current_process;
    process_t* new = k_malloc(sizeof(process_t));

    memcpy(new, src, sizeof(process_t));

	new->pid      = __k_proc_process_allocate_pid();
    new->state    = PROCESS_STATE_STARTING;

	new->wait_queue = list_create();

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

	new->fds.nodes = k_calloc(src->fds.size, sizeof(fd_t*));
	for(uint32_t i = 0; i < src->fds.size; i++) {
		fd_t* original_node = src->fds.nodes[i];
		if(!original_node) {
			new->fds.nodes[i] = 0;
		} else {
			new->fds.nodes[i] = k_malloc(sizeof(fd_t));
			memcpy(new->fds.nodes[i], original_node, sizeof(fd_t));
			new->fds.nodes[i]->node->links++;
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
	k_free(process->fds.nodes[fd]);

	process->fds.nodes[fd] = 0;
	UNLOCK(process->fds.lock);
}

void k_proc_process_sleep_on_queue(process_t* process, list_t* queue) {
	process->state = PROCESS_STATE_SLEEPING;
	list_push_back(queue, process);
	k_proc_process_yield();
}

void k_proc_process_wakeup_queue(list_t* queue) {
	while(queue->size > 0) {
		process_t* process = list_pop_back(queue);
		if(process->state != PROCESS_STATE_FINISHED) {
			process->state = PROCESS_STATE_RUNNING;
			k_proc_process_mark_ready(process);
		}
	}
}

void k_proc_process_exit(process_t* process, int code) {
	k_info("Process %s (%d) exited with code %d", process->name, process->pid, code);

	process->status = code;

	fd_list_t* list = &process->fds;
	for(uint32_t i = 0; i < list->size; i++){
		if(list->nodes[i]) {
			k_fs_vfs_close(list->nodes[i]->node);
			k_free(list->nodes[i]);
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

	if(process->node->parent) {
		process_t* parent = process->node->parent->value;
		if(parent) {
			k_proc_process_wakeup_queue(parent->wait_queue);
		}
	}

	k_proc_process_switch();
}

void k_proc_process_grow_heap(process_t* process, int32_t size){
	uint32_t old_pd = k_mem_paging_get_pd(1);
	if(size > 0) {
		k_mem_paging_map_region(process->image.heap + process->image.heap_size, 0, size, 0x7, 0);
		process->image.heap_size += size * 0x1000;
	} else {
		//TODO unmap
	}
	k_mem_paging_set_pd(old_pd, 1, 0);
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
	list_delete_element(process_list, process);
	tree_remove_node(process_tree, process->node);
	tree_free_node(process->node);
	list_free(process->wait_queue);
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
	wait_node_t* node = sleep_queue; 
	while(node && (node->seconds < seconds || (node->seconds == seconds && node->microseconds <= microseconds))) {
		sleep_queue = node->next;
		process_t* process = node->process;
		if(process->state != PROCESS_STATE_FINISHED) {
			process->state = PROCESS_STATE_RUNNING;
			k_proc_process_mark_ready(process);
		}
		node->process->wait_node = 0;
		k_free(node);
		node = sleep_queue;
	}
}

static uint64_t __k_proc_get_tsc_ticks() {
	uint64_t tsc_ticks = k_dev_timer_read_tsc();
	uint64_t speed     = k_dev_timer_get_core_speed();
	uint64_t base      = k_dev_timer_tsc_base();

	uint64_t ticks     = (tsc_ticks / speed) - base; 

	return ticks;
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

	wait_node_t* new_node  = malloc(sizeof(wait_node_t));
	new_node->process      = process;
	new_node->seconds      = cur_seconds;
	new_node->microseconds = cur_microseconds;
	process->wait_node 	   = new_node;

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
	return process->state == PROCESS_STATE_RUNNING || process->state == PROCESS_STATE_STARTING;
}

void k_proc_process_send_signal(process_t* process, int sig) {
	if(sig < 0 || sig >= MAX_SIGNAL) {
		return;
	}

	process->signal_queue |= (1 << sig);

	if(process != current_core->current_process && !k_proc_process_is_ready(process)) {
		k_proc_process_mark_ready(process);
	}
}

static int __k_proc_process_handle_signal(process_t* process, int sig, interrupt_context_t* ctx) {
	if(sig < 0 || sig >= MAX_SIGNAL) {
		return 1;
	}

	signal_t cfg = process->signals[sig];

	if(!cfg.handler) {
		uint8_t def = sig_defaults[sig];
		if(def == SIG_TERMINATE) {
			k_proc_process_exit(process, ((128 + sig) << 4) | sig);
			__builtin_unreachable();
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
	k_info("Returning from signal handler...");

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

	//TODO restart syscall
}
