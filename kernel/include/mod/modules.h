#ifndef __K_MOD_MODULES_H
#define __K_MOD_MODULES_H

#include "fs/vfs.h"
#include "kernel.h"
#include "multiboot.h"

typedef struct{
    const char* name;
    K_STATUS (*load)   ();
    K_STATUS (*unload) ();
}module_info_t;

#define MODULE(name, load, unload) \
    module_info_t __k_module_info = {name, load, unload}; 

K_STATUS k_mod_load_modules(multiboot_info_t* mb);
K_STATUS k_mod_load_ramdisk(uint32_t addr, uint32_t size);

#endif
