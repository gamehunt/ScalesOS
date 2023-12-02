#ifndef __FCNTL_H
#define __FCNTL_H

#define F_DUPFD 0
#define F_GETFD 1 
#define F_SETFD 2

int fcntl(int fd, int cmd, long arg);

#endif
