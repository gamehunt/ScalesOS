#include <mod/elf.h>
#include <string.h>
#include "kernel.h"
#include "mem/paging.h"

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

static K_STATUS __k_mod_elf_load_exec(Elf32_Ehdr* hdr){
    if(!hdr->e_phoff){
        return K_STATUS_ERR_GENERIC;
    }

    Elf32_Phdr* phdr = (Elf32_Phdr*) ((uint32_t) hdr + hdr->e_phoff);
    for(uint32_t i = 0; i < hdr->e_phnum; i++){
        if(phdr->p_type == PT_LOAD){
            k_mem_paging_map_region(phdr->p_vaddr, 0, phdr->p_memsz / 0x1000 + 1, 0x3, 0x0);
            memset((void*) phdr->p_vaddr, 0, phdr->p_memsz);
            memcpy((void*) phdr->p_vaddr, (void*) ((uint32_t) hdr + phdr->p_offset), phdr->p_filesz);
        }
        phdr = (Elf32_Phdr*) ((uint32_t) phdr + hdr->e_phentsize);
    }

    return K_STATUS_OK;
}

static K_STATUS __k_mod_elf_load_rel(Elf32_Ehdr* hdr){
    if(!hdr->e_shoff){
        return K_STATUS_ERR_GENERIC;
    }

    Elf32_Shdr* shdr = (Elf32_Shdr*) ((uint32_t) hdr + hdr->e_shoff);
    for(uint32_t i = 0; i < hdr->e_shnum; i++){
        //TODO process header
        shdr = (Elf32_Shdr*) ((uint32_t) shdr + hdr->e_shentsize);
    }

    return K_STATUS_OK;
}

K_STATUS k_mod_elf_load(void* file){
    Elf32_Ehdr* hdr = (Elf32_Ehdr*) file;
    if(!__k_mod_elf_check(hdr)){
        return K_STATUS_ERR_GENERIC;
    }

    if(hdr->e_type == ET_EXEC){
        return __k_mod_elf_load_exec(hdr);
    }

    if(hdr->e_type == ET_REL){
        return __k_mod_elf_load_rel(hdr);
    }

    return K_STATUS_OK;
}