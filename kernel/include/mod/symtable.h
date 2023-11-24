#ifndef __K_MOD_SYMTABLE_H
#define __K_MOD_SYMTABLE_H

#include <stdint.h>
#include "kernel.h"

typedef struct sym{
    uint32_t addr;
	char module[32];
    char name[];
}sym_t;

K_STATUS k_mod_symtable_init();
void*    k_mod_symtable_get_symbol(const char* name, const char* module);
sym_t*   k_mod_symtable_get_nearest_symbol(uint32_t addr);
sym_t*   k_mod_symtable_define_symbol(const char* name, const char* module, uint32_t addr);

#endif
