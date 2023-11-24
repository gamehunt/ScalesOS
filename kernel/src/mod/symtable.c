#include <mod/symtable.h>
#include <string.h>
#include "kernel.h"
#include "mem/heap.h"
#include "types/list.h"
#include "util/log.h"

static list_t* symbols;

extern void* symbols_start;
extern void* symbols_end;

K_STATUS k_mod_symtable_init(){
	symbols = list_create();
    
    sym_t* addr = (sym_t*) &symbols_start;
    while((uint32_t) addr < (uint32_t) &symbols_end){
        list_push_back(symbols, (void*) addr);
        addr = (sym_t*) (((uint32_t)addr) + sizeof(sym_t) + strlen(addr->name) + 1);
    }

    k_info("Exported %d symbols.", symbols->size);

    return K_STATUS_OK;
}

void* k_mod_symtable_get_symbol(const char* name, const char* module){
    for(uint32_t i = 0; i < symbols->size; i++){
		sym_t* s = symbols->data[i];
        if(!strcmp(name, s->name) && !strcmp(module, s->module)){
            return (void*) s->addr;
        }
    }
    return 0;
}

sym_t* k_mod_symtable_get_nearest_symbol(uint32_t addr){
    uint32_t difference = 0xFFFFFFFF;
    sym_t* result = 0;

    for(uint32_t i = 0; i < symbols->size; i++){
		sym_t* s = symbols->data[i];
        if(s->addr > addr){
            continue;
        }
        if(addr - s->addr < difference){
            difference = addr - s->addr;
            result = s;
        }
    }

    return result;
}

sym_t* k_mod_symtable_define_symbol(const char* name, const char* module, uint32_t addr) {
	sym_t* sym = k_malloc(sizeof(sym_t) + strlen(name) + 1);
	sym->addr = addr;
	strcpy(sym->module, module);
	strcpy(sym->name, name);
	list_push_back(symbols, sym);
	return sym;
}
