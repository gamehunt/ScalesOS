#include "mem/paging.h"
#include "mem/heap.h"
#include "mod/modules.h"
#include "mod/symtable.h"
#include "util/log.h"
#include <mod/elf.h>
#include <string.h>

uint8_t k_mod_elf_check(Elf32_Ehdr* hdr) {
    return hdr->e_ident[EI_MAG0] == ELFMAG0 &&
           hdr->e_ident[EI_MAG1] == ELFMAG1 &&
           hdr->e_ident[EI_MAG2] == ELFMAG2 &&
           hdr->e_ident[EI_MAG3] == ELFMAG3 &&
           hdr->e_ident[EI_CLASS] == ELFCLASS32 &&
           hdr->e_ident[EI_DATA] == ELFDATA2LSB && hdr->e_machine == EM_386 &&
           hdr->e_version == EV_CURRENT;
}


Elf32_Shdr* k_mod_elf_sheader(Elf32_Ehdr *hdr) {
	return (Elf32_Shdr *)((int)hdr + hdr->e_shoff);
}
 
Elf32_Shdr* k_mod_elf_section(Elf32_Ehdr *hdr, int idx) {
	return &k_mod_elf_sheader(hdr)[idx];
}

uint32_t k_mod_elf_get_symval(Elf32_Ehdr *hdr, int table, uint32_t idx) {
	if(table == SHN_UNDEF || idx == SHN_UNDEF) return 0;
	Elf32_Shdr* symtab = k_mod_elf_section(hdr, table);
 
	uint32_t symtab_entries = symtab->sh_size / symtab->sh_entsize;
	if(idx >= symtab_entries) {
		k_err("Symbol Index out of Range (%d:%u).\n", table, idx);
		return 0;
	}
 
	int symaddr = (int)hdr + symtab->sh_offset;
	Elf32_Sym *symbol = &((Elf32_Sym *)symaddr)[idx];

	if(symbol->st_shndx == SHN_UNDEF) {
		// External symbol, lookup value
		Elf32_Shdr* strtab = k_mod_elf_section(hdr, symtab->sh_link);
		const char* name = (const char *)hdr + strtab->sh_offset + symbol->st_name;
 
		void *target = k_mod_symtable_get_symbol(name);
 
		if(target == NULL) {
			// Extern symbol not found
			if(ELF32_ST_BIND(symbol->st_info) & STB_WEAK) {
				// Weak symbol initialized as 0
				return 0;
			} else {
				k_err("Undefined External Symbol : %s.\n", name);
				return 0;
			}
		} else {
			return (int)target;
		}
	} else if(symbol->st_shndx == SHN_ABS) {
		// Absolute symbol
		return symbol->st_value;
	} else {
		// Internally defined symbol
		Elf32_Shdr* target = k_mod_elf_section(hdr, symbol->st_shndx);
		return (int)hdr + symbol->st_value + target->sh_offset;
	}
}

#define DO_386_32(S, A)	((S) + (A))
#define DO_386_PC32(S, A, P)	((S) + (A) - (P))

uint32_t k_mod_elf_relocate(Elf32_Ehdr *hdr, Elf32_Rel *rel, Elf32_Shdr *reltab) {
	Elf32_Shdr* target =  k_mod_elf_section(hdr, reltab->sh_info);

	int addr = (int)hdr + target->sh_offset;
	int *ref = (int *)(addr + rel->r_offset);

	int symval = 0;
	if(ELF32_R_SYM(rel->r_info) != SHN_UNDEF) {
		symval = k_mod_elf_get_symval(hdr, reltab->sh_link, ELF32_R_SYM(rel->r_info));
		if(!symval) return 0;
	}

		// Relocate based on type
	switch(ELF32_R_TYPE(rel->r_info)) {
		case R_386_NONE:
			// No relocation
			break;
		case R_386_32:
			// Symbol + Offset
			*ref = DO_386_32(symval, *ref);
			break;
		case R_386_PC32:
			// Symbol + Offset - Section Offset
			*ref = DO_386_PC32(symval, *ref, (int)ref);
			break;
		default:
			// Relocation type not supported, display error and return
			k_err("Unsupported Relocation Type (%d).\n", ELF32_R_TYPE(rel->r_info));
			return 0;
	}
	return symval;
}

char* k_mod_elf_str_table(Elf32_Ehdr *hdr) {
	if(hdr->e_shstrndx == SHN_UNDEF) return NULL;
	return (char *)hdr + k_mod_elf_section(hdr, hdr->e_shstrndx)->sh_offset;
}
 
char* k_mod_elf_lookup_string(Elf32_Ehdr *hdr, int offset) {
	char *strtab = k_mod_elf_str_table(hdr);
	if(strtab == NULL) return NULL;
	return strtab + offset;
}

void* k_mod_elf_get_address(Elf32_Ehdr* hdr, Elf32_Sym* sym)
{
    Elf32_Shdr* target = k_mod_elf_section(hdr, sym->st_shndx);
    return (void*) ((uint32_t)hdr + target->sh_offset + sym->st_value);
}

module_info_t* k_mod_elf_load_module(void* file) {
    Elf32_Ehdr* hdr = (Elf32_Ehdr*)file;

    if (!k_mod_elf_check(hdr)) {
		k_err("Not an elf.");
        return 0;
    }

    if (hdr->e_type != ET_REL) {
		k_err("Not a relocatable.");
        return 0;
    }

    if (!hdr->e_shoff) {
		k_err("No symtable.");
        return 0;
    }

	module_info_t* mod = 0;

    Elf32_Shdr* shdr = k_mod_elf_sheader(hdr);
    for (uint32_t i = 0; i < hdr->e_shnum; i++) {
		if(shdr->sh_type == SHT_NOBITS) {
			if(shdr->sh_size && (shdr->sh_flags & SHF_ALLOC)) {
				void *mem = malloc(shdr->sh_size);
				memset(mem, 0, shdr->sh_size);
				shdr->sh_offset = (int)mem - (int)hdr;
				// k_debug("Allocated memory for a section (%ld).", shdr->sh_size);
			}
		}
		if(shdr->sh_type == SHT_REL) {
			for(uint32_t idx = 0; idx < shdr->sh_size / shdr->sh_entsize; idx++) {
				Elf32_Rel *reltab = &((Elf32_Rel *)((int)hdr + shdr->sh_offset))[idx];
				int result = k_mod_elf_relocate(hdr, reltab, shdr);
				if(!result) {
					k_err("Failed to relocate symbol.");
					return 0;
				} 
			}
		}
		if(shdr->sh_type == SHT_SYMTAB && !mod){
			Elf32_Sym* symbol = (Elf32_Sym*) ((uint32_t)hdr + shdr->sh_offset);
			Elf32_Shdr *strtab = k_mod_elf_section(hdr, shdr->sh_link);
			uint32_t offset = 0;
			while(offset < shdr->sh_size) {
				if(symbol->st_name) {
					const char *name = (const char *)hdr + strtab->sh_offset + symbol->st_name;
					if(!strcmp(name, "__k_module_info")) {
						mod = (module_info_t*) k_mod_elf_get_address(hdr, symbol);
						break;
					}
				}
				symbol++;
				offset += sizeof(Elf32_Sym);
			}

		}
    	shdr = (Elf32_Shdr*)((uint32_t)shdr + hdr->e_shentsize);
    }

    return mod;
}
