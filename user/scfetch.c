#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>

int main(int argc, char** argv) {
	struct utsname info;
	int result = uname(&info);

	if(result < 0) {
		printf("Failed to get system info.");
		return -errno;
	}

	
	printf("OS: %s\n", info.sysname);
	printf("Host: %s\n", info.machine);
	printf("Kernel: %s-%s\n", info.version, info.release);

	return 0;
}
