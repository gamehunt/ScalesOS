#ifndef __SYS_UTSNAME
#define __SYS_UTSNAME

struct utsname {
	char sysname[257];
	char nodename[257];
	char release[257];
	char version[257];
	char machine[257];
};

int uname(struct utsname *buf); 

#endif
