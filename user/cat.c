#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#include <sys/stat.h>

int main(int argc, char** argv) {
	if(argc < 2) {
		printf("Usage: cat <file>\n");
		return 1;
	}

	FILE* f = fopen(argv[1], "r");
	if(!f) {
		printf("No such file.\n");
		return 1;
	}

	struct stat sb;
	if(fstat(fileno(f), &sb) < 0) {
		printf("stat() failed with code %ld", errno);
		return 1;
	}

	size_t l = sb.st_size;
	char* buff = malloc(l);
	
	fread(buff, 1, l, f);
	fclose(f);
	
	for(size_t i = 0; i < l; i++) {
		putchar(buff[i]);
	}

	puts("\n");

	return 0;
}

