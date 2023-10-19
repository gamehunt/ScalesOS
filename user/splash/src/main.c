#include "sys/mman.h"
#include "sys/tty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fb.h>

#include <kernel/dev/fb.h>

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

	fb_t fb;
	int r = fb_open("/dev/fb", &fb);
	if(r > 0) {
		fb_fill(&fb, 0x0000FF);
		fb_filled_rect(&fb, 50, 50, 100, 50, 0xFF0000, 0x00FF00);
		fb_flush(&fb);
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
