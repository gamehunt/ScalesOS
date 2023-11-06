#include "sys/tty.h"
#include "sys/ioctl.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void usage() {

}

int main(int argc, char** argv) {
	if(argc < 2) {
		usage();
		return 0;
	}

	char path[256];
	int n = atoi(argv[1]);
	snprintf(path, 256, "/dev/vt%d", n);

	int vt = open(path, O_RDONLY);
	if(vt < 0) {
		return 1;
	}

	ioctl(vt, VT_ACTIVATE, NULL);

	return 0;
}
