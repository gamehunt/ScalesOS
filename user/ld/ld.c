#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/heap.h>
#include <unistd.h>

#include <kernel/mod/elf.h>
#include <kernel/mem/memory.h>

#define ALIGN(value, alignment) \
    (((value) + (alignment)) & ~(((typeof(value))(alignment)) - 1))

typedef struct object {
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
	char**      dependencies;
	size_t      dependency_count;
} object_t;

object_t** loaded_libraries;
uint32_t   library_count = 0;

void add_loaded_library(object_t* lib) {
	if(!library_count) {
		library_count = 1;
		loaded_libraries = malloc(sizeof(object_t*));
	} else {
		library_count++;
		loaded_libraries = realloc(loaded_libraries, library_count * sizeof(object_t*));
	}

	loaded_libraries[library_count - 1] = lib;
}

extern void __resolver_wrapper();

uint32_t lookup_symbol(object_t* lib, const char* name) {
	for(uint32_t i = 0; i < lib->dyn_symtab_size; i++) {
		Elf32_Sym sym = lib->dyn_symtab[i];
		char* sname = (char*) (lib->dyn_str_table + sym.st_name);
		if(!strcmp(sname, name)) {
			return lib->base + sym.st_value;
		}
	}
	return 0;
}

uint32_t resolve_symbol(object_t* object, uint32_t index) {
	printf("Resolver called: 0x%.8x with index 0x%x\n", (uint32_t) object, index);

	Elf32_Rel* symbol_rel_entry = (Elf32_Rel*) ((uint32_t)object->relplt + index);

	printf("Symbol: 0x%.8x 0x%.8x (index %d)\n", symbol_rel_entry->r_offset, symbol_rel_entry->r_info, ELF32_R_SYM(symbol_rel_entry->r_info));

	Elf32_Sym symbol = object->dyn_symtab[ELF32_R_SYM(symbol_rel_entry->r_info)];

	char* name = (char*) (object->dyn_str_table + symbol.st_name);

	printf("Resolving symbol %s\n", name);

	for(int i = 0; i < library_count; i++) {
		object_t* lib = loaded_libraries[i];
		if(lib == object) {
			continue;
		}
		uint32_t addr = lookup_symbol(lib, name);
		if(addr) {
			printf("Resolved at 0x%.8x\n", addr);
			*((uint32_t*) (object->base + symbol_rel_entry->r_offset)) = addr;
			return addr;
		}
	}

	printf("Failed to resolve symbol: %s\n", name);

	exit(-1);
}

void try_relocate(object_t* object, Elf32_Rel* rel, size_t size) {
	for(size_t i = 0; i < size; i++) {
		Elf32_Rel data   = rel[i];	
		Elf32_Sym symbol = object->dyn_symtab[ELF32_R_SYM(data.r_info)];

		char* name = object->dyn_str_table + symbol.st_name;
    	
		if(symbol.st_value) {
			switch(ELF32_R_TYPE(data.r_info)) {
				case R_386_GLOB_DAT:
				case R_386_32:
				case R_386_JMP_SLOT:
					*((uint32_t*)(object->base + data.r_offset)) = object->base + symbol.st_value;
					break;
				default:
					printf("LD: Unknown relocation type: %d\n", ELF32_R_TYPE(data.r_info));
			}
		}
	}
}

object_t* load_object(object_t* object, uint8_t absolute) {
	Elf32_Ehdr* header = object->file;

	if (header->e_ident[0] != ELFMAG0 ||
	    header->e_ident[1] != ELFMAG1 ||
	    header->e_ident[2] != ELFMAG2 ||
	    header->e_ident[3] != ELFMAG3) {
		printf("Not an elf executable. (Got: 0x%.2x %c %c %c)\n",
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
		object->base = ALIGN((uint32_t) object->file + object->file_size, 0x1000) + 0x1000;
	}
	object->end = object->base;

	uint32_t base = object->base;

	// Process program headers
	uint32_t h = 0;
	while(h < header->e_phnum) {
		Elf32_Phdr* phdr = (Elf32_Phdr*) (((uint32_t) header) + header->e_phoff + h * header->e_phentsize);
		void* mem = 0;
		switch(phdr->p_type) {
			case PT_LOAD:
				mem = mmap((void*) (object->base + phdr->p_vaddr & 0xFFFFF000), phdr->p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_FIXED | MAP_ANONYMOUS, 0, 0);
				if((int32_t) mem == MAP_FAILED) {
					printf("mmap(): failed to map section 0x%.8x - 0x%.8x (%ld)\n", base + phdr->p_vaddr, base + phdr->p_vaddr + phdr->p_memsz, errno);
				} else {
					memset(mem, 0, phdr->p_memsz);
					printf("mmap(): mapped section 0x%.8x - 0x%.8x\n", base + phdr->p_vaddr, base + phdr->p_vaddr + phdr->p_memsz);
					if(object->end < base + phdr->p_vaddr + phdr->p_memsz) {
						object->end = base + phdr->p_vaddr + phdr->p_memsz;
					}
					memcpy((void*) (base + phdr->p_vaddr), (void*) (((uint32_t) header) + phdr->p_offset), phdr->p_filesz);
				}
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
				printf("GOT address: 0x%.8x\n", &got[0]);
				printf("GOT argument address: 0x%.8x\n", &got[1]);
				printf("GOT resolver address: 0x%.8x\n", &got[2]);
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
					printf("DT_HASH\n");
					break;
				case DT_STRTAB:
					object->dyn_str_table = (char *)(base + table->d_un.d_ptr);
					printf("DT_STRTAB\n");
					break;
				case DT_SYMTAB:
					printf("DT_SYMTAB\n");
					object->dyn_symtab = (Elf32_Sym*) (base + table->d_un.d_ptr);
					break;
				case DT_STRSZ:
					object->dyn_str_table_size = table->d_un.d_val;
					printf("DT_STRSZ\n");
					break;
				case DT_INIT: 
					printf("DT_INIT\n");
					if(table->d_un.d_ptr) {
						object->init = (void (*)(void))(table->d_un.d_ptr + base);
					}
					break;
				case DT_INIT_ARRAY: 					
					printf("DT_ARRAY\n");
					if(table->d_un.d_ptr) {
						object->init_array = (void (**)(void))(table->d_un.d_ptr + base);
					}
					break;
				case DT_INIT_ARRAYSZ: 	
					printf("DT_ARRAYSZ\n");
					object->init_array_size = table->d_un.d_val / sizeof(uintptr_t);
					break;
			}
			table++;	
		}

		table = object->dynamic;
		object->dependency_count = 0;
		object->dependencies = malloc(sizeof(char*));
		while (table->d_tag) {
			char* name;
			switch (table->d_tag) {
				case DT_NEEDED:
					name = object->dyn_str_table + table->d_un.d_val;
					printf("Needed: %s\n", name);
					object->dependencies[object->dependency_count] = strdup(name);
					object->dependency_count++;
					object->dependencies = realloc(object->dependencies, sizeof(char*) * (object->dependency_count + 1));
					break;
			}
			table++;
		}
	}

	// Do relocations
	
	// if(object->dyn_symtab) {
	// 	for(int i = 0; i < object->dyn_symtab_size; i++) {
	// 		object->dyn_symtab[i].st_value += base;
	// 	}
	// }

	if(object->relplt) {
		try_relocate(object, object->relplt, object->relplt_size);
	}	

	if(object->reldyn) {
		try_relocate(object, object->reldyn, object->reldyn_size);
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

	printf("Mapped %s to 0x%.8x - 0x%.8x\n", path, object->file, (uint32_t) object->file + object->file_size);

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

	printf("Mapped %s to 0x%.8x - 0x%.8x\n", path, object->file, (uint32_t) object->file +object->file_size);

	return load_object(object, 1);
}

extern char** environ;
extern void __jump(uint32_t entry, int argc, const char** argv, int envc, char** envp);

int main(int argc, const char** argv) {

	object_t* main = load_exec(argv[1]);
	if(!main) {
		return -1;
	}
	
	add_loaded_library(main);

	for(int i = 0; i < main->dependency_count; i++) {
		printf("Loading: %s\n", main->dependencies[i]);
		object_t* lib = load_library(main->dependencies[i]);
		if(!lib) {
			printf("Failed to load library %s!\n", main->dependencies[i]);
			exit(-1);
		}
		add_loaded_library(lib);
	}

	uint32_t heap_start = ALIGN(main->end, 0x1000) + 0x1000;
	void* heap = mmap((void*) heap_start, USER_HEAP_INITIAL_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_FIXED | MAP_ANONYMOUS, 0, 0);

	if((int32_t) heap == MAP_FAILED || setheap(heap) < 0) {
		printf("LD: Failed to allocate heap.\n");
		return -1;
	}

	printf("Moved heap to 0x%.8x\n", heap_start);

	for(int i = 1; i < library_count; i++) {
		object_t* lib = loaded_libraries[i];

		if(lib->init_array) {
			for(int j = 0; j < lib->init_array_size; j++) {
				lib->init_array[j]();
			}
		}

		if(lib->init) {
			printf("Calling init() at 0x%.8x\n", (uint32_t) lib->init);
			lib->init();
		}
	}

	if(main->init_array) {
		for(int j = 0; j < main->init_array_size; j++) {
			main->init_array[j]();
		}
	}

	if(main->init) {
		printf("Calling init() at 0x%.8x\n", (uint32_t) main->init);
		main->init();
	}

	printf("Jumping to 0x%.8x\n", main->file->e_entry);
	fflush(stdout);

	__jump(main->file->e_entry, argc - 2, argv + 2, 0, environ);

	return 0;
}
