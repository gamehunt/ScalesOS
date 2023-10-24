#include <stdio.h>
#include <sys/stat.h>

void usage() {

}

int main(int argc, char** argv) {
	if(argc < 2) {
		usage();
	}
	return mkdir(argv[1], 0);
}
