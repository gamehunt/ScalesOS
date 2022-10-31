#ifndef __K_MOD_MODULES_H
#define __K_MOD_MODULES_H

#include "kernel.h"
#include "multiboot.h"

#define MOD_START 0xE0000000

K_STATUS k_mod_load_modules(multiboot_info_t* mb);

#endif