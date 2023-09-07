#ifndef __SYS_TTY_H
#define __SYS_TTY_H

#include <stdint.h>
#include "sys/types.h"

#define IGNBRK  (1 << 0)
#define BRKINT  (1 << 1)
#define IGNPAR  (1 << 2)
#define PARMRK  (1 << 3)
#define INPCK   (1 << 4)
#define ISTRIP  (1 << 5)
#define INLCR   (1 << 6)
#define IGNCR   (1 << 7)
#define ICRNL   (1 << 8)
#define IUCLC   (1 << 9)
#define IXON    (1 << 10)
#define IXANY   (1 << 11)
#define IMAXBEL (1 << 12)

#define OPOST   (1 << 0)
#define OLCUC   (1 << 1)  
#define ONLCR   (1 << 2)
#define OCRNL   (1 << 3)
#define ONOCR   (1 << 4)
#define ONLRET  (1 << 5)
#define OFILL   (1 << 6)
#define OFDEL   (1 << 7)
#define NLDLY   (1 << 8)
#define   NL0   0
#define   NL1   (1 << 8)
#define CRDLY   ((1 << 10) | (1 << 9))
#define   CR0   0
#define   CR1   (1 << 9)
#define   CR2   (1 << 10)
#define   CR3   ((1 << 10) | (1 << 9))
#define TABDLY  ((1 << 12) | (1 << 11))
#define  TAB0   0
#define  TAB1   (1 << 11)
#define  TAB2   (1 << 12)
#define  TAB3   ((1 << 12) | (1 << 11))
#define BSDLY   (1 << 13)
#define   BS0   0
#define   BS1   (1 << 13)
#define VTDLY   (1 << 14)
#define   VT0   0
#define   VT1   (1 << 14)
#define FFDLY   (1 << 15)
#define   FF0   0
#define   FF1   (1 << 15)

#define CBAUD    0x1F
#define CBAUDEX  0x3F
#define CSIZE    ((1 << 7) | (1 << 6))
#define   CS5    0
#define   CS6    (1 << 6)
#define   CS7    (1 << 7)
#define   CS8    ((1 << 7) | (1 << 6))
#define CSTOPB   (1 << 8)
#define CREAD    (1 << 9)
#define PARENB   (1 << 10)
#define PARODD   (1 << 11)
#define HUPCL    (1 << 12)
#define CLOCAL   (1 << 13)
#define LOBLK    (1 << 14)
#define CRTSCTS  (1 << 15)
#define CIBAUD   0x3F0000

#define B0        0
#define B50       1
#define B75       2
#define B110      3
#define B134      4
#define B150      5
#define B200      6
#define B300      7
#define B600      8
#define B1200     9
#define B1800     10
#define B2400     11
#define B4800     12
#define B9600     13
#define B19200    14
#define B38400    15
#define B57600    16
#define B115200   17
#define B230400   18

#define ISIG     (1 << 0)
#define ICANON   (1 << 1)
#define XCASE    (1 << 2)
#define ECHO     (1 << 3)
#define ECHOE    (1 << 4)
#define ECHOK    (1 << 5)
#define ECHONL   (1 << 6)
#define ECHOCTL  (1 << 7)
#define ECHOPRT  (1 << 8)
#define ECHOKE   (1 << 9)
#define DEFECHO  (1 << 10)
#define FLUSHO   (1 << 11)
#define NOFLSH   (1 << 12)
#define TOSTOP   (1 << 13)
#define PENDIN   (1 << 14)
#define IEXTEN   (1 << 15)

#define VINTR    1 
#define VQUIT    2
#define VERASE   3
#define VKILL    4
#define VEOF     5
#define VMIN     6
#define VEOL     7
#define VTIME    8
#define VEOL2    9
#define VSWTCH   10
#define VSTART   11
#define VSTOP    12
#define VSUSP    13
#define VDSUSP   14
#define VLNEXT   15
#define VWERASE  16
#define VREPRINT 17
#define VDISCARD 18
#define VSTATUS  19

#define NCCS     20 

#define TCSANOW   0
#define TCSADRAIN 1
#define TCSAFLUSH 2

#define TCIFLUSH  0
#define TCOFLUSH  1
#define TCIOFLUSH 2

#define TCOOFF    0
#define TCOON     1
#define TCIOFF    2
#define TCION     3

// IOCTL

#define TCGETS    0x5401 // Get termios struct. args: struct termios *
#define TCSETS    0x5402 // Set termios struct. args: const struct termios *
#define TCSETSW   0x5043 // Set termios struct. drain first. args: const struct termios *
#define TCSETSF   0x5044 // Set termios struct. flush first. args: const struct termios *

#define TCGETA   TCGETS
#define TCSETA   TCSETS
#define TCGETAW  TCGETSW
#define TCGETAF  TCGETSF

#define VT_ACTIVATE	0x5606

struct termios {
	tcflag_t c_iflag;
	tcflag_t c_oflag;
	tcflag_t c_cflag;
	tcflag_t c_lflag;
	cc_t     c_cc[NCCS]; 
};

struct winsize {
	uint16_t ws_rows;
	uint16_t ws_cols;
};

#if !defined(__KERNEL) && !defined(__LIBK)

int openpty(int* master, int* slave, char* name, struct termios* params, struct winsize* ws);

int tcgetattr(int fd, struct termios *termios_p);
int tcsetattr(int fd, int optional_actions, struct termios *termios_p);
int tcsendbreak(int fd, int duration);
int tcdrain(int fd);
int tcflush(int fd, int queue_selector);
int tcflow(int fd, int action);
int cfmakeraw(struct termios *termios_p);

speed_t cfgetispeed(struct termios *termios_p);
speed_t cfgetospeed(struct termios *termios_p);

int cfsetispeed(struct termios *termios_p, speed_t speed);
int cfsetospeed(struct termios *termios_p, speed_t speed);  

#endif

#endif
