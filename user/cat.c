#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

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

	fseek(f, 0, SEEK_END);
	size_t l = ftell(f);
	
	fseek(f, 0, SEEK_SET);
	char* buff = malloc(l);
	
	fread(buff, 1, l, f);
	fclose(f);
	
	for(size_t i = 0; i < l; i++) {
		fputc(buff[i], stdout);
	}

	puts("\n");

	return 0;
}

