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

	FILE* console = fopen("/dev/console", "r");
	if(!console) {
		return 1;
	}

	int n = atoi(argv[1]);
	ioctl(fileno(console), VT_ACTIVATE, &n);

	return 0;
}
