#include "sys/mman.h"
#include "sys/tty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>

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

	FILE* fb = fopen("/dev/fb", "r+");
	if(fb) {
		uint32_t color = 0x236bb2;
		ioctl(fileno(fb), FB_IOCTL_CLEAR, &color);

		uint32_t  resolution = 1280 * 800 * 4;
		uint32_t* screen = mmap(NULL, resolution, PROT_READ | PROT_WRITE, MAP_SHARED, fileno(fb), 0);

		for(int i = 0; i < 1280; i++) {
			screen[50 * 1280 + i] = 0x00ff00;
		}

		for(int i = 0; i < 1280; i++) {
			for(int j = 0; j < 800; j++) {
				if(i == j) {
					screen[j * 1280 + i] = 0x0000ff;
				}
			}
		}

		for(int i = 0; i < 800; i++) {
			screen[i * 1280 + 10] = 0xff0000;
			screen[i * 1280 + 750] = 0xff0000;
		}

		msync(screen, resolution, MS_SYNC);
		ioctl(fileno(fb), FB_IOCTL_SYNC, NULL);
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
