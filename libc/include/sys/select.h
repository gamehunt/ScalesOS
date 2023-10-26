#ifndef __SYS_SELECT
#define __SYS_SELECT

#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>
#include <string.h>

#define FD_SETSIZE 1024

typedef struct {
	uint32_t bitmap[FD_SETSIZE / 32];
} fd_set;

int select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
int pselect(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, const struct timespec *timeout, const sigset_t* sigmask);

#define FD_CLR(fd, set) \
		(set)->bitmap[fd / 32] &= ~(1 << (fd % 32))

#define FD_ISSET(fd, set) \
		((set)->bitmap[fd / 32] & (1 << (fd % 32)))

#define FD_SET(fd, set) \
		(set)->bitmap[fd / 32] |= (1 << (fd % 32))

#define FD_ZERO(set) \
		memset((set)->bitmap, 0, sizeof(fd_set))

#endif
