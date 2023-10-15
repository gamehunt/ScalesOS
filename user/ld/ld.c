#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, const char** argv) {
	void* big = malloc(256 * 1024 * 1024);

	printf("Big allocation: 0x%.8x\n", (uint32_t) big);

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
		printf("mmap() allocated vm at 0x%.8x\n", (uint32_t) mem);

		char* elf = (char*) mem;
		printf("0x%.2x %c %c %c\n", elf[0], elf[1], elf[2], elf[3]);
	}


	return 0;
}
