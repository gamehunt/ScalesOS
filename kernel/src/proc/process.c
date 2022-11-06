#include <proc/process.h>
#include <string.h>
#include "dev/timer.h"
#include "int/isr.h"
#include "kernel.h"
#include "mem/heap.h"
#include "mem/paging.h"
#include "util/asm_wrappers.h"
#include "util/log.h"

#define  STACK_SIZE MB(1)

static process_t** processes;
static uint32_t    total_processes;
static uint32_t    current_process;

static void __k_proc_process_scheduler_callback(interrupt_context_t* ctx UNUSED){
    k_proc_process_yield();
}

static void __k_proc_process_idle(){
    while(1) {
        k_proc_process_yield();
        halt();
    }
}

void k_proc_process_spawn(process_t* proc){
    EXTEND(processes, total_processes, sizeof(process_t*));
    proc->pid = total_processes;
    processes[total_processes - 1] = proc;
}

static void __k_proc_process_create_init(){
    process_t* proc = k_malloc(sizeof(process_t));

    strcpy(proc->name, "[init]");

    proc->context.esp = 0;
    proc->context.ebp = 0;
    proc->context.eip = 0;  
    proc->context.page_directory = k_mem_paging_get_pd(0);

    k_proc_process_spawn(proc);
}

static void __k_proc_process_create_idle(){
    process_t* proc = k_malloc(sizeof(process_t));

    strcpy(proc->name, "[kidle]");

    proc->context.esp = ((uint32_t) k_calloc(STACK_SIZE, 0)) + STACK_SIZE;
    proc->context.ebp = proc->context.esp;
    proc->context.eip = (uint32_t) &__k_proc_process_idle;         
    proc->context.page_directory = k_mem_paging_get_pd(0);

    k_proc_process_spawn(proc);
}

void k_proc_process_init(){
    cli();

    k_d_mem_heap_print();
    k_debug("");

    total_processes = 0;
    current_process = 0;

    k_dev_timer_add_callback(__k_proc_process_scheduler_callback);

    __k_proc_process_create_init();
    __k_proc_process_create_idle();

    k_d_mem_heap_print();

    sti();
}

extern __attribute__((returns_twice)) uint8_t __k_proc_process_save(context_t* ctx);
extern __attribute__((noreturn))      void __k_proc_process_load(context_t* ctx);

void k_proc_process_yield(){
    if(!total_processes){
        return;
    }

    uint32_t prev_process = current_process;

    current_process++;
    if(current_process >= total_processes){
        current_process = 0;
    }

    process_t* old_proc = processes[prev_process];
    process_t* new_proc = processes[current_process];

    /*k_debug("Switching from [%d] %s to [%d] %s", old_proc->pid, 
                                                 old_proc->name,
                                                 new_proc->pid, 
                                                 new_proc->name);

    k_info("Stack occupied in %s: %d", old_proc->name, old_proc->context.ebp - old_proc->context.esp);
    k_info("Stack occupied in %s: %d", new_proc->name, new_proc->context.ebp - new_proc->context.esp);*/

    if(old_proc->pid != 2){ // No need in saving idle task context
        if(__k_proc_process_save(&old_proc->context)){
            return; // Just returned from switch, do nothing and let it return
        }
    }

   __k_proc_process_load(&new_proc->context);
}