#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

int main(int argc, const char** argv) {
	void* big = malloc(256 * 1024 * 1024);

	printf("Big allocation: 0x%.8x\n", (uint32_t) big);

	FILE* file = fopen(argv[1], "r");

	if(!file) {
		printf("LD: no such file: %s\n", argv[1]);
		return -1;
	}

	void* mem = mmap(0, fseek(file, 0, SEEK_END), PROT_READ, MAP_PRIVATE, fileno(file), 0);

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
