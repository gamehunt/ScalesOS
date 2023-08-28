#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>

static void usage() {

}

int main(int argc, char** argv) {
	if(argc < 2) {
		usage();
		return 1;
	}

	umount(argv[1]);

	return 0;
}
