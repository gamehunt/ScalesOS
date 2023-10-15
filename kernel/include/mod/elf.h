#ifndef __K_MOD_ELF_H
#define __K_MOD_ELF_H

#ifdef __KERNEL
	#include "mod/modules.h"
#else
	#include <kernel/mod/modules.h>
#endif

#define EI_MAG0    0 // File identification
#define EI_MAG1    1 // File identification
#define EI_MAG2    2 // File identification
#define EI_MAG3    3 // File identification
#define EI_CLASS   4 // File class
#define EI_DATA    5 // Data encoding
#define EI_VERSION 6 // File version
#define EI_PAD     7 // Start of padding bytes
#define EI_NIDENT  16

#define ELFMAG0 0x7f // e_ident[EI_MAG0]
#define ELFMAG1 'E'  // e_ident[EI_MAG1]
#define ELFMAG2 'L'  // e_ident[EI_MAG2]
#define ELFMAG3 'F'  // e_ident[EI_MAG3]

#define ELFCLASSNONE 0 // Invalid class
#define ELFCLASS32   1 // 32-bit objects
#define ELFCLASS64   2 // 64-bit objects

#define ELFDATANONE 0
#define ELFDATA2LSB 1 
#define ELFDATA2MSB 2 

#define ET_NONE 0        // No file type
#define ET_REL  1        // Relocatable file
#define ET_EXEC 2        // Executable file
#define ET_DYN  3        // Shared object file
#define ET_CORE 4        // Core file
#define ET_LOPROC 0xff00 // Processor-specific
#define ET_HIPROC 0xffff // Processor-specific

#define EM_NONE  0 // No machine
#define EM_M32   1 // AT&T WE 32100
#define EM_SPARC 2 // SPARC
#define EM_386   3 // Intel 80386
#define EM_68K   4 // Motorola 68000
#define EM_88K   5 // Motorola 88000
#define EM_860   7 // Intel 80860
#define EM_MIPS  8 // MIPS RS3000

#define EV_NONE    0 // Invalid version
#define EV_CURRENT 1 // Current version

#define SHN_UNDEF     0
#define SHN_LORESERVE 0xff00
#define SHN_LOPROC    0xff00
#define SHN_HIPROC    0xff1f
#define SHN_ABS       0xfff1
#define SHN_COMMON    0xfff2
#define SHN_HIRESERVE 0xffff

#define SHT_NULL     0
#define SHT_PROGBITS 1
#define SHT_SYMTAB   2
#define SHT_STRTAB   3
#define SHT_RELA     4
#define SHT_HASH     5
#define SHT_DYNAMIC  6
#define SHT_NOTE     7
#define SHT_NOBITS   8
#define SHT_REL      9
#define SHT_SHLIB    10
#define SHT_DYNSYM   11
#define SHT_LOPROC   0x70000000
#define SHT_HIPROC   0x7fffffff
#define SHT_LOUSER   0x80000000
#define SHT_HIUSER   0xffffffff

#define SHF_WRITE     0x1
#define SHF_ALLOC     0x2
#define SHF_EXECINSTR 0x4
#define SHF_MASKPROC  0xf0000000

#define ELF32_ST_BIND(i)   ((i)>>4)
#define ELF32_ST_TYPE(i)   ((i)&0xf)
#define ELF32_ST_INFO(b,t) (((b)<<4)+((t)&0xf)) 

#define STB_LOCAL  0
#define STB_GLOBAL 1
#define STB_WEAK   2
#define STB_LOPROC 13
#define STB_HIPROC 15

#define STT_NOTYPE  0
#define STT_OBJECT  1
#define STT_FUNC    2
#define STT_SECTION 3
#define STT_FILE    4
#define STT_LOPROC  13
#define STT_HIPROC  15

#define ELF32_R_SYM(i)   ((i)>>8)
#define ELF32_R_TYPE(i)   ((unsigned char) (i))
#define ELF32_R_INFO(s,t) (((s)<<8)+(unsigned char)(t)) 

#define R_386_NONE     0  // none none
#define R_386_32       1  // word32 S + A
#define R_386_PC32     2  // word32 S + A - P
#define R_386_GOT32    3  // word32 G + A - P
#define R_386_PLT32    4  // word32 L + A - P
#define R_386_COPY     5  // none none
#define R_386_GLOB_DAT 6  // word32 S
#define R_386_JMP_SLOT 7  // word32 S
#define R_386_RELATIVE 8  // word32 B + A
#define R_386_GOTOFF   9  // word32 S + A - GOT
#define R_386_GOTPC    10 // word32 GOT + A - P

#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6
#define PT_LOPROC  0x70000000
#define PT_HIPROC  0x7fffffff

#define DT_NULL         0
#define DT_NEEDED       1
#define DT_PLTRELSZ     2
#define DT_PLTGOT       3
#define DT_HASH         4
#define DT_STRTAB       5
#define DT_SYMTAB       6
#define DT_RELA         7
#define DT_RELASZ       8
#define DT_RELAENT      9
#define DT_STRSZ        10
#define DT_SYMENT       11
#define DT_INIT         12
#define DT_FINI         13
#define DT_SONAME       14
#define DT_RPATH        15
#define DT_SYMBOLIC     16
#define DT_REL          17
#define DT_RELSZ        18
#define DT_RELENT       19
#define DT_PLTREL       20
#define DT_DEBUG        21
#define DT_TEXTREL      22
#define DT_JMPREL       23
#define DT_BIND_NOW     24
#define DT_INIT_ARRAY   25
#define DT_FINI_ARRAY   26
#define DT_INIT_ARRAYSZ 27
#define DT_FINI_ARRAYSZ 28
#define DT_LOOS   0x60000000
#define DT_HIOS   0x6FFFFFFF
#define DT_LOPROC 0x70000000
#define DT_HIPROC 0x7FFFFFFF


#include <stdint.h>

typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef int32_t  Elf32_Sword;
typedef uint32_t Elf32_Word;

typedef struct {
    unsigned char e_ident [EI_NIDENT];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
} Elf32_Ehdr;

typedef struct {
    Elf32_Word sh_name;
    Elf32_Word sh_type;
    Elf32_Word sh_flags;
    Elf32_Addr sh_addr;
    Elf32_Off  sh_offset;
    Elf32_Word sh_size;
    Elf32_Word sh_link;
    Elf32_Word sh_info;
    Elf32_Word sh_addralign;
    Elf32_Word sh_entsize;
} Elf32_Shdr; 

typedef struct {
    Elf32_Word    st_name;
    Elf32_Addr    st_value;
    Elf32_Word    st_size;
    unsigned char st_info;
    unsigned char st_other;
    Elf32_Half    st_shndx;
} Elf32_Sym; 

typedef struct {
    Elf32_Addr r_offset;
    Elf32_Word r_info;
} Elf32_Rel;

typedef struct {
    Elf32_Addr  r_offset;
    Elf32_Word  r_info;
    Elf32_Sword r_addend;
} Elf32_Rela; 

typedef struct {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
} Elf32_Phdr; 

typedef struct {
        Elf32_Sword d_tag;
        union {
                Elf32_Word      d_val;
                Elf32_Addr      d_ptr;
                Elf32_Off       d_off;
        } d_un;
} Elf32_Dyn;

module_info_t*     k_mod_elf_load_module(void* file);

uint8_t     k_mod_elf_check(Elf32_Ehdr* hdr); 
Elf32_Shdr* k_mod_elf_sheader(Elf32_Ehdr *hdr);
Elf32_Shdr* k_mod_elf_section(Elf32_Ehdr *hdr, int idx);
uint32_t    k_mod_elf_get_symval(Elf32_Ehdr *hdr, int table, uint32_t idx);
uint32_t    k_mod_elf_relocate(Elf32_Ehdr *hdr, Elf32_Rel *rel, Elf32_Shdr *reltab);
char*       k_mod_elf_str_table(Elf32_Ehdr *hdr);
char*       k_mod_elf_lookup_string(Elf32_Ehdr *hdr, int offset);
void*       k_mod_elf_get_address(Elf32_Ehdr* hdr, Elf32_Sym* sym);

#endif
