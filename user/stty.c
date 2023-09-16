#include "sys/tty.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void sane(struct termios* ts) {
	ts->c_iflag = ICRNL | BRKINT;
	ts->c_oflag = ONLCR | OPOST;
	ts->c_lflag = ECHO | ECHOE | ECHOK | ICANON | ISIG | IEXTEN;
	ts->c_cflag = CREAD | CS8;
	ts->c_cc[VEOF]    =  4;   // ^D 
	ts->c_cc[VEOL]    =  0;   // 
	ts->c_cc[VERASE]  = 0x7f; // ^? 
	ts->c_cc[VINTR]   =  3;   // ^C 
	ts->c_cc[VKILL]   = 21;   // ^U 
	ts->c_cc[VMIN]    =  1;
	ts->c_cc[VQUIT]   = 28;   // ( ^\ ) 
	ts->c_cc[VSTART]  = 17;   // ^Q 
	ts->c_cc[VSTOP]   = 19;   // ^S 
	ts->c_cc[VSUSP]   = 26;   // ^Z 
	ts->c_cc[VTIME]   =  0;
	ts->c_cc[VLNEXT]  = 22;   // ^V 
	ts->c_cc[VWERASE] = 23;   // ^W 
}

static void usage() {

}

int main(int argc, char** argv) {
	if(argc < 2) {
		usage();
		return 0;
	}
	
	struct termios t;
	tcgetattr(STDERR_FILENO, &t);
	
	for(int i = 2; i < argc; i++) {
		if(!strcmp(argv[0], "sane")) {
			sane(&t);
			continue;
		}
	}

	tcsetattr(STDERR_FILENO, TCSAFLUSH, &t);
	return 0;
}
