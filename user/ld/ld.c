#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <kernel/mod/elf.h>

typedef struct {

} executable_t;

executable_t* load(Elf32_Ehdr* header) {
	if (header->e_ident[0] != ELFMAG0 ||
	    header->e_ident[1] != ELFMAG1 ||
	    header->e_ident[2] != ELFMAG2 ||
	    header->e_ident[3] != ELFMAG3) {
		printf("Not an elf executable.");
		return NULL;
	}

	uint32_t h = 0;
	while(h < header->e_phnum) {
		Elf32_Phdr* phdr = (Elf32_Phdr*) (((uint32_t) header) + header->e_phoff + h * header->e_phentsize);
		void* mem = 0;
		switch(phdr->p_type) {
			case PT_LOAD:
				mem = mmap((void*) (phdr->p_vaddr & 0xFFFFF000), phdr->p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_FIXED | MAP_ANONYMOUS, 0, 0);
				if((int32_t) mem == MAP_FAILED) {
					printf("mmap(): failed to map section 0x%.8x - 0x%.8x (%ld)\n", phdr->p_vaddr, phdr->p_vaddr + phdr->p_memsz, errno);
				} else {
					printf("mmap(): mapped section 0x%.8x - 0x%.8x\n", phdr->p_vaddr, phdr->p_vaddr + phdr->p_memsz);
					memcpy((void*) phdr->p_vaddr, (void*) (((uint32_t) header) + phdr->p_offset), phdr->p_memsz);
				}
				break;
			case PT_DYNAMIC:

				break;
		}

		h++;
	}

	return NULL;
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
	} 

	Elf32_Ehdr* elf = (Elf32_Ehdr*) mem;
	executable_t* exec = load(elf);

	int (*entry)(int, const char**, char**);

	entry = (void*) elf->e_entry;
	printf("Jumping to 0x%.8x\n", elf->e_entry);
	entry(argc - 1, argv + 1, environ);

	return 0;
}
