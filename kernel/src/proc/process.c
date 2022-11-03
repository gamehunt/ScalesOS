#include <proc/process.h>
#include <string.h>
#include "dev/timer.h"
#include "int/isr.h"
#include "mem/gdt.h"
#include "mem/paging.h"

static process_t** processes;
static uint32_t    total_processes;
static uint32_t    current_process;

static void __k_proc_process_scheduler_callback(interrupt_context_t* ctx){
    if(!total_processes){
        return;
    }

    uint32_t prev_process = current_process;

    current_process++;
    if(current_process >= total_processes){
        current_process = 0;
    }

    k_proc_process_save_context(processes[prev_process], *ctx);
    k_proc_process_setup_context(processes[current_process], ctx);
}

void k_proc_process_init(){
    total_processes = 0;
    current_process = 0;

    k_dev_timer_add_callback(__k_proc_process_scheduler_callback);
}

void k_proc_process_save_context(process_t* process, interrupt_context_t ctx){
    process->context = ctx;
}

void k_proc_process_setup_context(process_t* process, interrupt_context_t* ctx){
    k_mem_paging_set_pd(process->page_directory, 0);
    k_mem_gdt_set_stack(process->kernel_stack);
    *ctx = process->context;
}