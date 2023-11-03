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
#include <fb_colors.h>
#include <tga.h>

#include <kernel/dev/fb.h>

static FILE* pipe;

static fb_t       fb;
static fb_font_t* font;

// https://colorhunt.co/palette/30384100adb5eeeeeeff5722
#define COLOR_BACKGROUND 0XFF303841
#define COLOR_BORDERS    0XFF00ADB5
#define COLOR_TEXT       0xFFEEEEEE
#define BORDER_HEIGHT    50
#define BAR_HEIGHT       15
#define BAR_COLOR        COLOR_BORDERS

static size_t strwidth(const char* str) {
	return strlen(str) * font->width;
}

static void draw(int stage, const char* status) {
	fb_fill(&fb, COLOR_BACKGROUND);
	fb_filled_rect(&fb, 0, 0, fb.info.w, BORDER_HEIGHT, COLOR_BORDERS, COLOR_BORDERS);
	fb_filled_rect(&fb, 0, fb.info.h - BORDER_HEIGHT - 1, fb.info.w, fb.info.h, COLOR_BORDERS, COLOR_BORDERS);

	size_t sw = strwidth(status);

	fb_string(&fb, fb.info.w / 2 - sw / 2, fb.info.h / 2 - font->height, status, font, 0x0, COLOR_TEXT);

	if(stage > 0) {
		double max_width     = fb.info.w / 3;
		double actual_width  = max_width * stage / 10;
		fb_filled_rect(&fb, fb.info.w / 3, fb.info.h / 2 + font->height + 20, actual_width, BAR_HEIGHT, BAR_COLOR, BAR_COLOR);
	}
}

int main(int argc, char** argv) {

	int pipefd = mkfifo("/tmp/splash", 0);
	
	if(!(pipe = fdopen(pipefd, "r"))) {
		return -1;
	}

	setbuf(pipe, 0);

	int vt = open("/dev/vt7", O_RDONLY);
	if(!vt) {
		return -1;
	}

	ioctl(vt, VT_ACTIVATE, NULL);
	ioctl(vt, KDSETMODE, (void*) VT_DISP_MODE_GRAPHIC);
	close(vt);

	int r = fb_open("/dev/fb", &fb, FB_FLAG_DOUBLEBUFFER);
	if(r < 0) {
		return -2;
	}	

	r = fb_open_font("/res/fonts/system.psf", &font);
	if(r < 0) {
		return -3;
	}	

	if(fork()) {
		return 0;
	}

	char str[256] = {0};
	int  l = 0;

	int op    = 0;
	int s     = 0;

	draw(0, "Loading...");
	fb_flush(&fb);

	while(1) {
		char c = fgetc(pipe);
		if(!op && c == '!') {
			op = 1;
			continue;
		} else if(op) {
			op = 0;
			if(c == 'q') {
				break;
			} else if(c == 's') {
				s = fgetc(pipe) - '0';
				continue;
			}
		}
		str[l] = c;
		if(c == '\n' || l == sizeof(str) - 1) {
			draw(s ,str);
			fb_flush(&fb);

			l = 0;
			memset(str, 0, sizeof(str));
		} else {
			l++;
		}
	}

	fclose(pipe);
	remove("/tmp/splash");

	return 0;
}
