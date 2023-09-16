#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mod.h>

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

	fseek(mod, 0, SEEK_END);
	size_t l = ftell(mod);
	fseek(mod, 0, SEEK_SET);

	uint8_t* buff = malloc(l);
	fread(buff, 1, l, mod);

	return insmod(buff, l);
}
