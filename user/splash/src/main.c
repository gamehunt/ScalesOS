#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

static FILE* pipe;
static FILE* console;

int main(int argc, char** argv) {
	int pipefd = mkfifo("/tmp/splash", 0);
	
	if(!(pipe = fdopen(pipefd, "r"))) {
		return 1;
	}

	setbuf(pipe, 0);
	
	if(!(console = fopen("/dev/console", "w"))) {
		return 2;
	}
	
	if(!fork()) {
		fprintf(console, "Loading ScalesOS...\r\n");

		while(1) {
			fprintf(console, "%c", fgetc(pipe));	
		}
	}

	return 0;
}
