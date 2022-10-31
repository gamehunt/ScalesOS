#include <mod/elf.h>
#include "kernel.h"

static uint8_t __k_mod_elf_check(Elf32_Ehdr* hdr){
    return hdr->e_ident[EI_MAG0]    == ELFMAG0     &&
           hdr->e_ident[EI_MAG1]    == ELFMAG1     &&
           hdr->e_ident[EI_MAG2]    == ELFMAG2     &&
           hdr->e_ident[EI_MAG3]    == ELFMAG3     &&
           hdr->e_ident[EI_CLASS]   == ELFCLASS32  &&
           hdr->e_ident[EI_DATA]    == ELFDATA2LSB &&
           hdr->e_machine           == EM_386      &&
           hdr->e_version           == EV_CURRENT;
}

K_STATUS k_mod_elf_load(void* file){
    Elf32_Ehdr* hdr = (Elf32_Ehdr*) file;
    if(!__k_mod_elf_check(hdr)){
        return K_STATUS_ERR_GENERIC;
    }

    return K_STATUS_OK;
}