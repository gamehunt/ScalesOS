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

#include "dev/pci.h"
#include "dev/timer.h"
#include "fs/tar.h"
#include "mem/heap.h"
#include "mem/pmm.h"
#include "mod/modules.h"
#include "mod/symtable.h"
#include "util/path.h"

void kernel_main(uint32_t magic UNUSED, multiboot_info_t* mb) {
    k_dev_serial_init();

    k_info("Booting Scales V%s...", KERNEL_VERSION);

    k_mem_gdt_init();

    k_int_pic_init(0x20, 0x28);
    k_int_idt_init();
    k_int_isr_init();
    k_int_irq_init();

    k_mem_pmm_init(mb);
    k_mem_paging_init();
    k_mem_heap_init();

    k_dev_pci_init();

    k_dev_pit_init();
    k_dev_timer_init();

    k_fs_vfs_init();
    k_fs_tar_init();

    k_fs_vfs_create_entry("/dev");

    k_mod_symtable_init();
    k_mod_load_modules(mb);

    k_fs_vfs_mount("/", "/dev/ram0", "tar");

    fs_node_t* dir = k_fs_vfs_open("/ramdisk/modules");

    if(dir){
        k_info("Contents: ");
        uint32_t i = 0;
        struct dirent* dent = k_fs_vfs_readdir(dir, i);
        while(dent){
            k_info("%s %d", dent->name, dent->ino);
            i++;
            dent = k_fs_vfs_readdir(dir, i);
        }
    }else{
        k_info("Not found.");
    }

    k_fs_vfs_close(dir);

    while (1) {
        halt();
    }
}