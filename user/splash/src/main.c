#include "sys/tty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>

static FILE* pipe;
static FILE* console;

int main(int argc, char** argv) {
	int pipefd = mkfifo("/tmp/splash", 0);
	
	if(!(pipe = fdopen(pipefd, "r"))) {
		return 1;
	}

	setbuf(pipe, 0);

	FILE* console = fopen("/dev/console", "r");
	if(!console) {
		return 0;
	}

	int n = 7;
	ioctl(fileno(console), VT_ACTIVATE, &n);

	FILE* fb = fopen("/dev/fb", "w");
	if(fb) {
		uint32_t  resolution = 1200 * 1080;
		uint32_t* screen = malloc(resolution * 4);
		for(int i = 0; i < resolution; i++) {
			screen[i] = 0x236bb2;
		}
		fwrite(screen, 4, resolution, fb);
		fflush(fb);
		free(screen);
	}
	
	if(!(console = fopen("/dev/tty0", "w"))) {
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
