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

	int seconds = atoi(argv[1]);

	sleep(seconds);

	return 0;
}
