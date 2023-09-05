#include "sys/tty.h"
#include "sys/ioctl.h"
#include "sys/syscall.h"
#include "errno.h"

int openpty(int* master, int* slave, char* name, struct termios* params, struct winsize* ws) {
	 __return_errno(__sys_openpty(master, slave, name, params, ws));
}

int tcgetattr(int fd, struct termios *termios_p) {
	return ioctl(fd, TCGETS, termios_p);
}

int tcsetattr(int fd, int optional_actions, struct termios *ts) {
	switch (optional_actions) {
		case TCSANOW:
			return ioctl(fd, TCSETS, ts);
		case TCSADRAIN:
			return ioctl(fd, TCSETSW, ts);
		case TCSAFLUSH:
			return ioctl(fd, TCSETSF, ts);
		default:
			return 0;
	}
}

int tcsendbreak(int fd, int duration) {
	return 0;
}

int tcdrain(int fd) {
	return 0;
}

int tcflush(int fd, int queue_selector) {
	return 0;
}

int tcflow(int fd, int action) {
	return 0;
}

int cfmakeraw(struct termios *termios_p) {
	return 0;
}

speed_t cfgetispeed(struct termios *termios_p) {
	return 0;
}

speed_t cfgetospeed(struct termios *termios_p) {
	return 0;
}

int cfsetispeed(struct termios *termios_p, speed_t speed) {
	return 0;
}

int cfsetospeed(struct termios *termios_p, speed_t speed) {
	return 0;
}
