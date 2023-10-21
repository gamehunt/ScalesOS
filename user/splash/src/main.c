#include "sys/tty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fb.h>

#include <kernel/dev/fb.h>

static FILE* pipe;
static FILE* console;

int main(int argc, char** argv) {

	int pipefd = mkfifo("/tmp/splash", 0);
	
	if(!(pipe = fdopen(pipefd, "r"))) {
		return -1;
	}

	setbuf(pipe, 0);

	FILE* console = fopen("/dev/console", "r");
	if(!console) {
		return -1;
	}

	int n = 7;
	ioctl(fileno(console), VT_ACTIVATE, &n);

	fb_t fb;
	int r = fb_open("/dev/fb", &fb);
	if(r < 0) {
		return -2;
	}	

	fb_font_t* font;
	r = fb_open_font("/res/fonts/system.psf", &font);
	if(r < 0) {
		return -3;
	}	

	if(fork()) {
		return 0;
	}

	int  op = 0;
	char str[256] = {0};
	int  l = 0;

	int y = 0;

	fb_fill(&fb, 0x0000CC);
	fb_flush(&fb);

	while(1) {
		char c = fgetc(pipe);
		if(op && c == 'q') {
			break;
		} else if(!op && c == '!') {
			op = 1;
			continue;
		} else if(op) {
			op = 0;
		}
		str[l] = c;
		if(c == '\n' || l == sizeof(str) - 1) {
			fb_fill(&fb, 0x0000CC);
			fb_string(&fb, 0, y + 10, str, font, 0x0000CC, 0xFF0000);
			fb_flush(&fb);

			y += font->height;
			l = 0;
			memset(str, 0, sizeof(str));
		} else {
			l++;
		}
	}

	return 0;
}
