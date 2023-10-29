#include "types/list.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/heap.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <kernel/mod/elf.h>
#include <kernel/mem/memory.h>

#define ALIGN(value, alignment) \
    (((value) + (alignment)) & ~(((typeof(value))(alignment)) - 1))

typedef struct object {
	char*       name;
	Elf32_Ehdr* file;
	uint32_t    file_size;
	uint32_t    base;
	uint32_t    end;
	Elf32_Dyn*  dynamic;
	Elf32_Rel*  relplt;
	size_t      relplt_size;
	Elf32_Rel*  reldyn;
	size_t      reldyn_size;
	Elf32_Sym*  dyn_symtab;
	size_t      dyn_symtab_size;
	uint32_t    str_table;
	char*       dyn_str_table;
	size_t      dyn_str_table_size;
	void        (*init)(void);
	void        (**init_array)(void);
	size_t      init_array_size;
	list_t*     dependencies;
} object_t;

typedef struct {
	char*    name;
	uint32_t addr;
	uint32_t src;
} overriden_symbol_t;

static list_t*   loaded_libraries = NULL;
static list_t*   symbol_overrides = NULL;
static object_t* __main = NULL;

overriden_symbol_t* get_symbol_override(char* name) {
	for(int i = 0; i < symbol_overrides->size; i++) {
		overriden_symbol_t* s = symbol_overrides->data[i];
		if(!strcmp(name, s->name)) {
			return s;
		}
	}
	return 0;
}

uint8_t is_library_loaded(char* name) {
	for(int i = 0; i < loaded_libraries->size; i++) {
		object_t* lib = loaded_libraries->data[i];
		if(!strcmp(lib->name, name)) {
			return 1;
		}
	}
	return 0;
}

void add_loaded_library(object_t* lib) {
	list_push_back(loaded_libraries, lib);
}

extern void __resolver_wrapper();

uint32_t lookup_symbol(object_t* lib, const char* name, object_t* requester) {
	if(!lib) {
		for(int i = 0; i < loaded_libraries->size; i++) {
			if(loaded_libraries->data[i] == requester) {
				continue;
			}
			uint32_t addr = lookup_symbol(loaded_libraries->data[i], name, requester);
			if(addr) {
				return addr;
			}
		}
		return 0;
	}
	for(uint32_t i = 0; i < lib->dyn_symtab_size; i++) {
		Elf32_Sym sym = lib->dyn_symtab[i];
		char* sname = (char*) (lib->dyn_str_table + sym.st_name);
		if(!strcmp(sname, name) && sym.st_value) {
			return lib->base + sym.st_value;
		}
	}
	return 0;
}

uint32_t resolve_symbol(object_t* object, uint32_t index) {
	Elf32_Rel* symbol_rel_entry = (Elf32_Rel*) ((uint32_t)object->relplt + index);

	Elf32_Sym symbol = object->dyn_symtab[ELF32_R_SYM(symbol_rel_entry->r_info)];

	char* name = (char*) (object->dyn_str_table + symbol.st_name);

	uint32_t addr = lookup_symbol(NULL, name, object);

	if(addr) {
		*((uint32_t*) (object->base + symbol_rel_entry->r_offset)) = addr;
		return addr;
	}

	printf("LD: Failed to resolve symbol: %s\n", name);

	exit(-1);
}

void object_relocate_section(object_t* object, Elf32_Rel* rel, size_t size) {
	for(size_t i = 0; i < size; i++) {
		Elf32_Rel data   = rel[i];	
		Elf32_Sym symbol = object->dyn_symtab[ELF32_R_SYM(data.r_info)];

		char* name = object->dyn_str_table + symbol.st_name;

		uint32_t* symbol_location = (uint32_t*)(object->base + data.r_offset);
		uint32_t  symbol_address  = 0;
		overriden_symbol_t* ovs   = get_symbol_override(name);

		if(!ovs) {
			symbol_address = object->base + symbol.st_value;
		} else {
			symbol_address = ovs->addr;
		}

		switch(ELF32_R_TYPE(data.r_info)) {
			case R_386_GLOB_DAT:
			case R_386_JMP_SLOT:
				if(symbol_address == object->base) {
					break;
				}
				*symbol_location = symbol_address;
				break;
			case R_386_32:
				if(symbol_address == object->base) {
					if(ELF32_ST_BIND(symbol.st_info) & STB_WEAK) {
						break;
					} else {
						printf("Unresolved symbol: %s\n", name);
						exit(-1);
					}
				}
				(*symbol_location) += symbol_address;
				break;
			case R_386_PC32:
				(*symbol_location) += symbol_address - ((uint32_t) symbol_location);
				break;
			case R_386_RELATIVE:
				(*symbol_location) += object->base;
				break;
			case R_386_COPY:
				if(!ovs) {
					printf("Copy without source? This should not be.\n");
					exit(-1);
				}
				memcpy(symbol_location, (void*) ovs->src, symbol.st_size);
				break;
			default:
				printf("LD: Unknown relocation type: %d\n", ELF32_R_TYPE(data.r_info));
				exit(-1);
		}
	}
}

void object_relocate_copies(object_t* object) {
	if(!object->reldyn) {
		return;
	}
	for(size_t i = 0; i < object->reldyn_size; i++) {
		Elf32_Rel data   = object->reldyn[i];	
		Elf32_Sym symbol = object->dyn_symtab[ELF32_R_SYM(data.r_info)];

		char* name = object->dyn_str_table + symbol.st_name;
		
		if(ELF32_R_TYPE(data.r_info) == R_386_COPY) {
			uint32_t sym = lookup_symbol(NULL, name, object);
			if(!sym) {
				printf("LD: failed to resolve symbol for copy: %s\n", name);
				exit(-1);
			}

			overriden_symbol_t* s = malloc(sizeof(overriden_symbol_t));
			s->name = name;
			s->addr = object->base + data.r_offset;
			s->src  = sym;
			list_push_back(symbol_overrides, s);
		}
	}
}

void relocate_object(object_t* object) {
	if(object->reldyn) {
		object_relocate_section(object, object->reldyn, object->reldyn_size);
	}
	
	if(object->relplt) {
		object_relocate_section(object, object->relplt, object->relplt_size);
	}
}

void relocate_objects() {
	for(int i = 0; i < loaded_libraries->size; i++) {
		object_t* lib = loaded_libraries->data[i];
		object_relocate_copies(lib);
	}

	for(int i = 1; i < loaded_libraries->size; i++) {
		object_t* lib = loaded_libraries->data[i];
		relocate_object(lib);
	}

	relocate_object(__main);
}

static uint32_t object_map_base(object_t* object) {
	Elf32_Ehdr* header = object->file;
	uint32_t    h    = 0;
	uint32_t    size = 0;
	while(h < header->e_phnum) {
		Elf32_Phdr* phdr = (Elf32_Phdr*) (((uint32_t) header) + header->e_phoff + h * header->e_phentsize);
		switch(phdr->p_type) {
			case PT_LOAD:
				if(phdr->p_vaddr + phdr->p_memsz > size) {
					size = phdr->p_vaddr + phdr->p_memsz;
				}
				break;
		}
		h++;
	}
	return (uint32_t) mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS, 0, 0);
}

object_t* load_library(const char* path);
object_t* load_object(object_t* object, uint8_t absolute) {
	Elf32_Ehdr* header = object->file;

	if (header->e_ident[0] != ELFMAG0 ||
	    header->e_ident[1] != ELFMAG1 ||
	    header->e_ident[2] != ELFMAG2 ||
	    header->e_ident[3] != ELFMAG3) {
		printf("LD: Not an elf executable. (Got: 0x%.2x %c %c %c)\n",
							header->e_ident[0],
							header->e_ident[1],
							header->e_ident[2],
							header->e_ident[3]
		);
		return NULL;
	}

	if(absolute) {
		object->base = 0;
	} else {
		object->base = object_map_base(object);
		if((int32_t)object->base == MAP_FAILED) {
			printf("LD: mmap() failed with code %ld\n", errno);
			exit(-1);
		}
	}
	object->end = object->base;

	uint32_t base = object->base;

	// Process program headers
	uint32_t h = 0;
	while(h < header->e_phnum) {
		Elf32_Phdr* phdr  = (Elf32_Phdr*) (((uint32_t) header) + header->e_phoff + h * header->e_phentsize);
		uint32_t    start = 0;
		switch(phdr->p_type) {
			case PT_LOAD:
				start = object->base + phdr->p_vaddr;
				if(!object->base) {
					int32_t mem = (int32_t)
						mmap((void*) (start & 0xFFFFF000), phdr->p_memsz + (start % 0x1000), PROT_READ | PROT_WRITE | PROT_EXEC, 
																							 MAP_FIXED | MAP_ANONYMOUS, 0, 0);
					if(mem == MAP_FAILED) {
							printf("LD: mmap() failed with code %ld\n", errno);
							exit(-1);
					}
				}
				memset((void*) start, 0, phdr->p_memsz);
				uint32_t real_start = (start & 0xFFFFF000);
				uint32_t real_size  = ALIGN(phdr->p_memsz + (start % 0x1000), 0x1000) + 0x1000;
				if(object->end < real_start + real_size) {
					object->end = real_start + real_size;
				}
				memcpy((void*) start, (void*) (((uint32_t) header) + phdr->p_offset), phdr->p_filesz);
				break;
			case PT_DYNAMIC:
				object->dynamic = (Elf32_Dyn*) (base + phdr->p_vaddr);
				break;
		}
		h++;
	}

	// Process dynamic section, and collect dependencies
	if(object->dynamic) {
		uint32_t s = 0;

		Elf32_Shdr* strtab = (Elf32_Shdr*) (((uint32_t) header) + header->e_shoff + header->e_shstrndx * header->e_shentsize);
		object->str_table = (((uint32_t) header) + strtab->sh_offset);
		
		while(s < header->e_shnum) {
			Elf32_Shdr* shdr = (Elf32_Shdr*) (((uint32_t) header) + header->e_shoff + s * header->e_shentsize);
			char* name = (char*) (object->str_table + shdr->sh_name);
			if(!strcmp(name, ".got.plt")) {
				uint32_t* got = (uint32_t*) (base + shdr->sh_addr);
				got[1] = (uint32_t) object;
				got[2] = (uint32_t) &__resolver_wrapper;
				for(int i = 3; i < shdr->sh_size / sizeof(uint32_t); i++) {
					got[i] += base;
				}
			} else if(!strcmp(name, ".rel.plt")) {
				object->relplt = (Elf32_Rel*) (base + shdr->sh_addr);
				object->relplt_size = shdr->sh_size / sizeof(Elf32_Rel);
			} else if(!strcmp(name, ".dynsym")) {
				object->dyn_symtab_size = shdr->sh_size / sizeof(Elf32_Sym);
			} else if(!strcmp(name, ".rel.dyn")) {
				object->reldyn = (Elf32_Rel*) (base + shdr->sh_addr);
				object->reldyn_size = shdr->sh_size / sizeof(Elf32_Rel);
			}
			s++;
		}

		Elf32_Dyn* table = object->dynamic;
		while(table->d_tag) {
			switch(table->d_tag) {
				case DT_HASH:
					break;
				case DT_STRTAB:
					object->dyn_str_table = (char *)(base + table->d_un.d_ptr);
					break;
				case DT_SYMTAB:
					object->dyn_symtab = (Elf32_Sym*) (base + table->d_un.d_ptr);
					break;
				case DT_STRSZ:
					object->dyn_str_table_size = table->d_un.d_val;
					break;
				case DT_INIT: 
					if(table->d_un.d_ptr) {
						object->init = (void (*)(void))(table->d_un.d_ptr + base);
					}
					break;
				case DT_INIT_ARRAY: 					
					if(table->d_un.d_ptr) {
						object->init_array = (void (**)(void))(table->d_un.d_ptr + base);
					}
					break;
				case DT_INIT_ARRAYSZ: 	
					object->init_array_size = table->d_un.d_val / sizeof(uintptr_t);
					break;
			}
			table++;	
		}

		table = object->dynamic;
		object->dependencies = list_create();
		while (table->d_tag) {
			char* name;
			switch (table->d_tag) {
				case DT_NEEDED:
					name = object->dyn_str_table + table->d_un.d_val;
					list_push_back(object->dependencies, strdup(name));
					break;
			}
			table++;
		}
	}

	// Do relocations
	add_loaded_library(object);

	for(int i = 0; i < object->dependencies->size; i++) {
		if(is_library_loaded(object->dependencies->data[i])) {
			continue;
		}
		object_t* lib = load_library(object->dependencies->data[i]);
		if(!lib) {
			printf("LD: Failed to load library %s\n", object->dependencies->data[i]);
			exit(-1);
		}
	}

	return object;
}

FILE* find_library(const char* path) {
	if(path[0] == '/') {
		return fopen(path, "r");
	}
	char full_path[1024];
	snprintf(full_path, 1024, "/lib/%s", path);
	return fopen(full_path, "r");
}

object_t* open_object(FILE* file) {
	struct stat sb;
	if(fstat(fileno(file), &sb) < 0) {
		printf("LD: stat() failed with code %ld\n", errno);
		return NULL;
	}
	void* mem = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, fileno(file), 0);
	if((int32_t) mem == MAP_FAILED) {
		printf("LD: mmap() failed with code %ld\n", errno);
		return NULL;
	} 

	object_t* obj = calloc(1, sizeof(object_t));
	obj->file = mem;
	obj->file_size = sb.st_size;

	return obj;
}

object_t* load_library(const char* path) {
	FILE* file = find_library(path);
	if(!file) {
		printf("LD: failed to find library %s\n", path);
		return NULL;
	}

	object_t* object = open_object(file);

	if(!object) {
		printf("LD: failed to open object %s\n", path);
		return NULL;
	}

	object->name = strdup(path);

	return load_object(object, 0);
}

object_t* load_exec(const char* path) {
	FILE* file = find_library(path);
	if(!file) {
		printf("LD: failed to find exec %s\n", path);
		return NULL;
	}

	object_t* object = open_object(file);
	if(!object) {
		printf("LD: failed to open object %s\n", path);
		return NULL;
	}

	object->name = strdup(path);

	return load_object(object, 1);
}

void init_objects() {
	for(int i = 1; i < loaded_libraries->size; i++) {
		object_t* lib = loaded_libraries->data[i];

		if(lib->init_array) {
			for(int j = 0; j < lib->init_array_size; j++) {
				lib->init_array[j]();
			}
		}

		if(lib->init) {
			lib->init();
		}
	}

	if(__main->init_array) {
		for(int j = 0; j < __main->init_array_size; j++) {
			__main->init_array[j]();
		}
	}

	if(__main->init) {
		__main->init();
	}
}

extern char** environ;
extern void __jump(uint32_t entry, int argc, const char** argv, int envc, char** envp);

int main(int argc, const char** argv) {
	setheapopts(HEAP_OPT_USE_MMAP);

	loaded_libraries = list_create();
	symbol_overrides = list_create();

	__main = load_exec(argv[1]);
	if(!__main) {
		return -1;
	}

	relocate_objects();


	uint32_t heap_start = ALIGN(__main->end, 0x1000) + 0x1000;
	void* heap = mmap((void*) heap_start, USER_HEAP_INITIAL_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_FIXED | MAP_ANONYMOUS, 0, 0);

	if((int32_t) heap == MAP_FAILED || setheap(heap) < 0) {
		printf("LD: Failed to allocate heap.\n");
		return -1;
	}

	init_objects();

	__jump(__main->file->e_entry, argc - 2, argv + 2, 0, environ);

	return 0;
}
