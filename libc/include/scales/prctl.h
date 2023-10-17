#ifndef __PRCTL_H
#define __PRCTL_H

#define PRCTL_SETNAME 0

int prctl(int op, void* arg);

#endif
