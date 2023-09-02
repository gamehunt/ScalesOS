#ifndef __SYS_TTY_H
#define __SYS_TTY_H

#include <stdint.h>

struct termios {

};

struct winsize {
	uint16_t ws_rows;
	uint16_t ws_cols;
};

#if !defined(__KERNEL) && !defined(__LIBK)

int openpty(int* master, int* slave, char* name, struct termios* params, struct winsize* ws);

#endif

#endif
