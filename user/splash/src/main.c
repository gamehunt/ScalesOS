#include "sys/tty.h"
#include <stdio.h>
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

	FILE* rnd = fopen("/dev/random", "r");
	if(!rnd) {
		return 0;
	}

	FILE* console = fopen("/dev/console", "r");
	if(!console) {
		return 0;
	}

	int n = 7;
	ioctl(fileno(console), VT_ACTIVATE, &n);

	FILE* fb = fopen("/dev/fb", "w");
	if(fb) {
		for(int i = 0; i < 1024 * 640 * 32; i++) {
			uint8_t byte;
			fread(&byte, 1, 1, rnd);
			fwrite(&byte, 1, 1, fb);
			fflush(fb);
		} 
	}
	
	// if(!(console = fopen("/dev/tty0", "w"))) {
	// 	return 2;
	// }
	// 
	// if(!fork()) {
	// 	fprintf(console, "Loading ScalesOS...\r\n");
	//
	// 	uint8_t op = 0;
	// 	while(1) {
	// 		char c = fgetc(pipe);
	// 		if(op && c == 'q') {
	// 			break;
	// 		} else if(!op && c == '!') {
	// 			op = 1;
	// 			continue;
	// 		} else if(op) {
	// 			op = 0;
	// 			fprintf(console, "!");
	// 		}
	// 		fprintf(console, "%c", c);	
	// 	}
	// }

	return 0;
}
