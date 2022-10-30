#include <mod/modules.h>
#include <string.h>
#include "fs/ramdisk.h"
#include "kernel.h"
#include "util/log.h"

K_STATUS k_mod_load_modules(multiboot_info_t* mb){
    multiboot_module_t* modules = (multiboot_module_t*) (mb->mods_addr + 0xC0000000);
    for(uint32_t i = 0; i < mb->mods_count; i++){
        uint32_t start  = modules->mod_start + 0xC0000000;
        uint32_t end    = modules->mod_end + 0xC0000000;
        const char* cmd = (const char*) (modules[i].cmdline + 0xC0000000);
        k_info("Module: 0x%.8x - 0x%.8x (CMD: %s)", start, end, cmd);
        if(!strcmp(cmd, "ramdisk")){
            k_fs_ramdisk_load(start, "/ramdisk");
        }
    }
    return K_STATUS_OK;
}