#include <proc/process.h>
#include <stdlib.h>
#include <string.h>
#include "fs/vfs.h"
#include "kernel.h"
#include "mem/gdt.h"
#include "mem/heap.h"
#include "mem/paging.h"
#include "mod/elf.h"
#include "util/asm_wrappers.h"
#include "util/log.h"

#define KERNEL_STACK_SIZE MB(1)
#define USER_STACK_SIZE   MB(4)
#define USER_STACK_START  0x9000000

static process_t** processes;
static uint32_t    total_processes;
static uint32_t    current_process;
static uint32_t    next_process;

static void __k_proc_process_idle(){
    while(1) {
        sti();
        halt();
        cli();
    }
}

void k_proc_process_spawn(process_t* proc){
    EXTEND(processes, total_processes, sizeof(process_t*));
    proc->pid = total_processes;
    processes[total_processes - 1] = proc;
    k_info("Process spawned: %s (%d)", proc->name, proc->pid);
}

static void __k_proc_process_create_init(){
    process_t* proc = k_calloc(1, sizeof(process_t));

    strcpy(proc->name, "[kernel]");

    proc->context.esp = 0;
    proc->context.ebp = 0;
    proc->context.eip = 0;  
    k_mem_paging_clone_pd(0, &proc->context.page_directory);

    k_proc_process_spawn(proc);
}

static void __k_proc_process_create_idle(){
    process_t* proc = k_calloc(1, sizeof(process_t));

    strcpy(proc->name, "[idle]");

    proc->context.esp = ((uint32_t) k_calloc(1, KERNEL_STACK_SIZE)) + KERNEL_STACK_SIZE;
    proc->context.ebp = proc->context.esp;
    proc->context.eip = (uint32_t) &__k_proc_process_idle;         
    k_mem_paging_clone_pd(0, &proc->context.page_directory);

    k_proc_process_spawn(proc);
}

void k_proc_process_init(){
    cli();

    total_processes = 0;
    current_process = 0;
    next_process    = 0;

    __k_proc_process_create_init();
    __k_proc_process_create_idle();

    sti();
}

extern __attribute__((returns_twice)) uint8_t __k_proc_process_save(context_t* ctx);
extern __attribute__((noreturn))      void    __k_proc_process_load(context_t* ctx);
extern __attribute__((noreturn))      void    __k_proc_process_enter_usermode(context_t* ctx, uint32_t userstack);

void k_proc_process_yield(){
    if(!total_processes){
        return;
    }

    uint32_t prev_process = current_process;

    current_process = next_process;
    next_process++;
    if(next_process >= total_processes){
        next_process = 0;
    }

    // k_info("Current: %d", current_process);

    process_t* old_proc = processes[prev_process];
    process_t* new_proc = processes[current_process];

    if(__k_proc_process_save(&old_proc->context)){
            return; // Just returned from switch, do nothing and let it return
    }

    uint8_t jump = new_proc->state == PROCESS_STATE_PL_CHANGE_REQUIRED;
    if(new_proc->state == PROCESS_STATE_STARTING || jump){
        new_proc->state = PROCESS_STATE_STARTED;
    }

    k_mem_gdt_set_stack(new_proc->context.esp);

    if(jump){
        __k_proc_process_enter_usermode(&new_proc->context, USER_STACK_START + USER_STACK_SIZE);
    }else{
        __k_proc_process_load(&new_proc->context);
    }
}

uint32_t k_proc_exec(const char* path, int argc UNUSED, char** argv UNUSED){
    if(!total_processes){
        return 0;
    }

    fs_node_t* node = k_fs_vfs_open(path);
    if(!node){
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
    k_mem_paging_clone_pd(0, &proc->context.page_directory);
    k_mem_paging_set_pd(proc->context.page_directory, 1, 0); 

    uint32_t entry;
    if((entry = k_mod_elf_load_exec(buffer))){
        k_free(buffer);
        proc->context.eip = entry;
        proc->context.esp = ((uint32_t) k_calloc(1, KERNEL_STACK_SIZE)) + KERNEL_STACK_SIZE;
        proc->context.ebp = proc->context.esp;
        k_mem_paging_map_region(USER_STACK_START, 0, USER_STACK_SIZE / 0x1000, 0x7, 0);
        k_proc_process_spawn(proc);
        proc->state = PROCESS_STATE_PL_CHANGE_REQUIRED;
    }else{
        k_free(proc);
    }

    k_mem_paging_set_pd(prev, 1, 0);

    sti();

    return proc->pid;
}

uint32_t k_proc_fork(){
    if(!total_processes){
        return 0;
    }

    cli();
    process_t* src = processes[current_process];
    process_t* new = k_malloc(sizeof(process_t));

    memcpy(new, src, sizeof(process_t));

    // -- TODO: 
    // 1) Proper copy of address space
    // 2) Save somewhere in the process syscall state
    // 3) Push it onto copied stack
    // 4) With helper function and iret return to the child with eax = 0

    /*uint32_t prev = k_mem_paging_get_pd(0);
    k_mem_paging_set_pd(new->context.page_directory, 1, 0);
    k_mem_paging_clone_pd(0, &new->context.page_directory);
    k_mem_paging_set_pd(prev, 1, 0);

    uint32_t* old_stack = (uint32_t*) (new->context.ebp - KERNEL_STACK_SIZE);
    uint32_t* new_stack = k_calloc(1, KERNEL_STACK_SIZE);

    uint32_t esp_diff = new->context.ebp - new->context.esp;

    memcpy(new_stack, old_stack, KERNEL_STACK_SIZE);

    new->context.ebp = ((uint32_t) new_stack + KERNEL_STACK_SIZE);
    new->context.esp = new->context.ebp - esp_diff;*/ 

    k_proc_process_spawn(new);

    sti();

    return new->pid;
}