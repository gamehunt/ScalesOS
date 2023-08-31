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
	
	if(!(console = fopen("/dev/console", "w"))) {
		return 2;
	}
	
	if(!fork()) {
		fprintf(console, "Loading ScalesOS...\r\n");

		while(1) {
			char line[256];
			while(fgets(line, 256, pipe)) {
				fprintf(console, "%s\r\n", line);	
			}
		}
	}

	return 0;
}
