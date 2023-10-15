#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <kernel/mod/elf.h>

#define ALIGN(value, alignment) \
    (((value) + (alignment)) & ~(((typeof(value))(alignment)) - 1))

typedef struct {
	uint32_t   base;
	uint32_t   end;
	Elf32_Dyn* dynamic;
	char*      dyn_str_table;
	size_t     dyn_str_table_size;
	char**     dependencies;
	size_t     dependency_count;
} object_t;

FILE* find_library(const char* path) {
	char full_path[1024];
	snprintf(full_path, 1024, "/lib/%s", path);
	return fopen(full_path, "r");
}

size_t calculate_object_size(Elf32_Ehdr* header) {
	int h = 0;
	uint32_t base = 0xFFFFFFFF;
	uint32_t end  = 0x0;
	while(h < header->e_phnum) {
		Elf32_Phdr* phdr = (Elf32_Phdr*) (((uint32_t) header) + header->e_phoff + h * header->e_phentsize);
		switch(phdr->p_type) {
			case PT_LOAD:
				if(phdr->p_vaddr < base) {
					base = phdr->p_vaddr;
				}
				if(phdr->p_vaddr + phdr->p_memsz > end) {
					end = phdr->p_vaddr + phdr->p_memsz;
				}
		}
		h++;
	}

	return (size_t) (end - base);
}


object_t* load(Elf32_Ehdr* header, uint32_t base) {
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

	object_t* exec = malloc(sizeof(object_t));
	exec->base = base;
	exec->end = base;

	uint32_t h = 0;
	while(h < header->e_phnum) {
		Elf32_Phdr* phdr = (Elf32_Phdr*) (((uint32_t) header) + header->e_phoff + h * header->e_phentsize);
		void* mem = 0;
		switch(phdr->p_type) {
			case PT_LOAD:
				mem = mmap((void*) (base + phdr->p_vaddr & 0xFFFFF000), phdr->p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_FIXED | MAP_ANONYMOUS, 0, 0);
				if((int32_t) mem == MAP_FAILED) {
					printf("mmap(): failed to map section 0x%.8x - 0x%.8x (%ld)\n", base + phdr->p_vaddr, base + phdr->p_vaddr + phdr->p_memsz, errno);
				} else {
					printf("mmap(): mapped section 0x%.8x - 0x%.8x\n", base + phdr->p_vaddr, base + phdr->p_vaddr + phdr->p_memsz);
					if(exec->end < base + phdr->p_vaddr + phdr->p_memsz) {
						exec->end = base + phdr->p_vaddr + phdr->p_memsz;
					}
					memcpy((void*) (base + phdr->p_vaddr), (void*) (((uint32_t) header) + phdr->p_offset), phdr->p_memsz);
				}
				break;
			case PT_DYNAMIC:
				exec->dynamic = (Elf32_Dyn*) (base + phdr->p_vaddr);
				break;
		}
		h++;
	}

	if(exec->dynamic) {
		Elf32_Dyn* table = exec->dynamic;
		while(table->d_tag) {
			switch(table->d_tag) {
				case DT_HASH:
					printf("DT_HASH\n");
					break;
				case DT_STRTAB:
					exec->dyn_str_table = (char *)(base + table->d_un.d_ptr);
					printf("DT_STRTAB\n");
					break;
				case DT_SYMTAB:
					printf("DT_SYMTAB\n");
					break;
				case DT_STRSZ:
					exec->dyn_str_table_size = table->d_un.d_val;
					printf("DT_STRSZ\n");
					break;
				case DT_INIT: 
					printf("DT_INIT\n");
					break;
				case DT_INIT_ARRAY: 					
					printf("DT_ARRAY\n");
					break;
				case DT_INIT_ARRAYSZ: 	
					printf("DT_ARRAYSZ\n");
					break;
			}
			table++;	
		}

		table = exec->dynamic;
		exec->dependency_count = 0;
		exec->dependencies = malloc(sizeof(char*));
		while (table->d_tag) {
			char* name;
			switch (table->d_tag) {
				case DT_NEEDED:
					name = exec->dyn_str_table + table->d_un.d_val;
					printf("Needed: %s\n", name);
					exec->dependencies[exec->dependency_count] = strdup(name);
					exec->dependency_count++;
					exec->dependencies = realloc(exec->dependencies, sizeof(char*) * (exec->dependency_count + 1));
					break;
			}
			table++;
		}
	}

	return exec;
}

object_t* preload_library(const char* path, uint32_t base) {
	FILE* file = find_library(path);
	if(!file) {
		printf("LD: failed to find library %s\n", path);
		return NULL;
	}

	struct stat sb;
	if(fstat(fileno(file), &sb) < 0) {
		printf("LD: stat() failed with code %ld\n", errno);
		return NULL;
	}

	void* mem = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, fileno(file), 0);
	if((int32_t) mem == MAP_FAILED) {
		printf("LD: mmap() failed with code %ld\n", errno);
		return NULL;
	} else {
		printf("Mapped %s to 0x%.8x\n", path, (uint32_t) mem);
	}

	return load(mem, base);
}

extern char** environ;

int main(int argc, const char** argv) {
	FILE* file = fopen(argv[1], "r");

	if(!file) {
		printf("LD: no such file: %s\n", argv[1]);
		return -1;
	}
	
	struct stat sb;
	if(fstat(fileno(file), &sb) < 0) {
		printf("LD: stat() failed with code %ld", errno);
		return -1;
	}

	void* mem = mmap(0, sb.st_size, PROT_READ, MAP_PRIVATE, fileno(file), 0);

	if((int32_t) mem == MAP_FAILED) {
		printf("LD: mmap() failed with code %ld\n", errno);
		return -1;
	} else {
		printf("Mapped %s to 0x%.8x\n", argv[1], (uint32_t) mem);
	}

	Elf32_Ehdr* elf = (Elf32_Ehdr*) mem;
	object_t* exec = load(elf, 0);

	uint32_t end = ALIGN(exec->end, 0x1000) + 0x1000;

	for(int i = 0; i < exec->dependency_count; i++) {
		object_t* lib = preload_library(exec->dependencies[i], end);
		if(lib) {
			printf("Loaded library: %s\n", exec->dependencies[i]);
			end = ALIGN(lib->end, 0x1000) + 0x1000;
		} else {
			printf("Failed to load: %s\n", exec->dependencies[i]);
		}
	}

	int (*entry)(int, const char**, char**);

	entry = (void*) elf->e_entry;
	
	printf("Jumping to 0x%.8x\n", elf->e_entry);

	munmap(mem, sb.st_size);

	entry(argc - 1, argv + 1, environ);

	return 0;
}
