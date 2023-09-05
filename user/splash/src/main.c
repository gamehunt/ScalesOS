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

		uint8_t op = 0;
		while(1) {
			char c = fgetc(pipe);
			if(op && c == 'q') {
				break;
			} else if(!op && c == '!') {
				op = 1;
				continue;
			} else if(op) {
				op = 0;
				fprintf(console, "!");
			}
			fprintf(console, "%c", c);	
		}
	}

	return 0;
}
