#ifndef __SYS_POLL
#define __SYS_POLL

#define POLLIN      0x0001
#define POLLPRI     0x0002
#define POLLOUT     0x0004
#define POLLERR     0x0008
#define POLLHUP     0x0010
#define POLLNVAL    0x0020

struct pollfd {
    int   fd;           
	short events;  
    short revents; 
};

int poll(struct pollfd *ufds, unsigned int nfds, int timeout);  

#endif
