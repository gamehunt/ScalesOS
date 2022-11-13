#include <dev/pit.h>
#include <dev/serial.h>
#include <int/idt.h>
#include <int/irq.h>
#include <int/isr.h>
#include <int/pic.h>
#include <mem/gdt.h>
#include <mem/paging.h>
#include <multiboot.h>
#include <shared.h>
#include <stdio.h>
#include <string.h>
#include <util/asm_wrappers.h>
#include <util/log.h>
#include <fs/vfs.h>
#include "dev/console.h"
#include "dev/fb.h"
#include "dev/pci.h"
#include "dev/timer.h"
#include "fs/tar.h"
#include "int/syscall.h"
#include "kernel.h"
#include "mem/heap.h"
#include "mem/pmm.h"
#include "mod/elf.h"
#include "mod/modules.h"
#include "mod/symtable.h"
#include "proc/process.h"
#include "util/panic.h"

extern void _libk_set_print_callback(void*);
extern void jump_userspace();

void kernel_main(uint32_t magic UNUSED, multiboot_info_t* mb) {
    k_dev_serial_init();

    k_info("Booting Scales V%s...", KERNEL_VERSION);

    k_mem_gdt_init();

    k_int_pic_init(0x20, 0x28);
    k_int_idt_init();
    k_int_isr_init();
    k_int_irq_init();
    k_int_syscall_init();

    k_mem_pmm_init(mb);
    k_mem_paging_init();
    k_mem_heap_init();

    k_fs_vfs_init();
    k_fs_vfs_create_entry("/dev");
    k_dev_console_init();

    k_dev_fb_init(mb);
    _libk_set_print_callback(k_dev_fb_write);
    k_dev_console_set_source(k_dev_fb_write);

    k_dev_pci_init();

    k_dev_pit_init();
    k_dev_timer_init();
    k_fs_tar_init();

    k_mod_symtable_init();
    k_mod_load_modules(mb);

   // k_proc_process_init();

    if(!IS_OK(k_fs_vfs_mount("/", "/dev/ram0", "tar"))){
        k_panic("Failed to mount root.", 0);
    }

    fs_node_t* node = k_fs_vfs_open("/init.sc");
    if(node){

        uint8_t* buffer = k_malloc(node->size);
        k_fs_vfs_read(node, 0, node->size, buffer);
        k_fs_vfs_close(node);


        uint32_t c = k_mem_paging_clone_pd(k_mem_paging_get_pd(0));
        k_mem_paging_set_pd(c, 0, 1); 

        uint32_t entry = k_mod_elf_load_exec(buffer);
        if(entry){
            k_info("ELF loaded.");

            uint32_t* stack = k_valloc(MB(1), 0x4);
            k_mem_gdt_set_stack((uint32_t) stack);
            k_mem_paging_map_region(0x9000000, 0, MB(1) / 0x1000, 0x7, 0);

            jump_usermode(entry);
        }else{
            k_err("Failed to load init.");
        }
        k_free(buffer);
    }else{
        k_err("Init not found.");
    }

    while (1) {
        halt();
    }
}