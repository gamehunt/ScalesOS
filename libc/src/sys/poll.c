#include "sys/time.h"
#include <sys/select.h>
#include <sys/poll.h>

int poll(struct pollfd *ufds, unsigned int nfds, int timeout) {
	fd_set r_set;
	FD_ZERO(&r_set);

	fd_set w_set;
	FD_ZERO(&w_set);

	fd_set e_set;
	FD_ZERO(&e_set);

	int n = 0;

	for(unsigned int i = 0; i < nfds; i++) {
		if(ufds->events & POLLIN) {
			FD_SET(ufds[i].fd, &r_set);
		} 
		if(ufds->events & POLLOUT) {
			FD_SET(ufds[i].fd, &w_set);
		} 
		if(ufds->events & POLLERR) {
			FD_SET(ufds[i].fd, &e_set);
		}
		if(ufds[i].fd > n) {
			n = ufds[i].fd;
		}
	}

	struct timeval tv;
	tv.tv_msec = timeout;

	int r = select(n + 1, &r_set, &w_set, &e_set, timeout > 0 ? &tv : NULL);

	if(r <= 0) {
		return r;
	}

	for(unsigned int i = 0; i < nfds; i++) {
		if(ufds->events & POLLIN && FD_ISSET(ufds[i].fd, &r_set)) {
			ufds->revents |= POLLIN;	
		}
		if(ufds->events & POLLOUT && FD_ISSET(ufds[i].fd, &w_set)) {
			ufds->revents |= POLLOUT;	
		}
		if(FD_ISSET(ufds[i].fd, &e_set)) {
			ufds->revents |= POLLERR;	
		}
	}

	return r;
}
