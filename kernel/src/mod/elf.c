#include <mod/elf.h>
#include "kernel.h"

static uint8_t __k_mod_elf_check(Elf32_Ehdr* hdr){
    return 0;
}

K_STATUS k_mod_elf_load(void* file){
    Elf32_Ehdr* hdr = (Elf32_Ehdr*) file;
    if(!__k_mod_elf_check(hdr)){
        return K_STATUS_ERR_GENERIC;
    }

    return K_STATUS_OK;
}