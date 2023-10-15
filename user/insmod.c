#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mod.h>
#include <sys/stat.h>

static void usage() {

}

int main(int argc, char** argv) {
	if(argc < 2) {
		usage();
		return 1;
	}

	char path[255];
	snprintf(path, 255, "/bin/modules/%s.scmod", argv[1]);

	FILE* mod = fopen(path, "r");
	if(!mod) {
		printf("Failed to open module: %s", argv[1]);
		return 1;
	}

	struct stat sb;
	if(fstat(fileno(mod), &sb) < 0) {
		printf("stat() failed with code %ld", errno);
		return 1;
	}

	size_t l = sb.st_size;

	uint8_t* buff = malloc(l);
	fread(buff, 1, l, mod);

	return insmod(buff, l);
}
