#include <mod/symtable.h>
#include <string.h>
#include "mem/heap.h"
#include "kernel.h"
#include "util/log.h"

static sym_t** symbols;
static uint32_t sym_count;

extern void* symbols_start;
extern void* symbols_end;

K_STATUS k_mod_symtable_init(){
    sym_count = 0;
    
    sym_t* addr = (sym_t*) &symbols_start;
    while((uint32_t) addr < (uint32_t) &symbols_end){
        EXTEND(symbols, sym_count, sizeof(sym_t*));
        symbols[sym_count - 1] = addr;
        addr = (sym_t*) (((uint32_t)addr) + sizeof(sym_t) + strlen(addr->name) + 1);
    }

    k_info("Exported %d symbols.", sym_count);

    return K_STATUS_OK;
}

void* k_mod_symtable_get_symbol(const char* name){
    for(uint32_t i = 0; i < sym_count; i++){
        if(!strcmp(name, symbols[i]->name)){
            return (void*) symbols[i]->addr;
        }
    }
    return 0;
}

sym_t*   k_mod_symtable_get_nearest_symbol(uint32_t addr){
    uint32_t difference = 0xFFFFFFFF;
    sym_t* result = 0;

    for(uint32_t i = 0; i < sym_count; i++){
        if(symbols[i]->addr > addr){
            continue;
        }
        if(addr - symbols[i]->addr < difference){
            difference = addr - symbols[i]->addr;
            result = symbols[i];
        }
    }

    return result;
}