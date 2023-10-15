#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int main(int argc, char** argv) {
	FILE* motd = fopen("/etc/motd", "r");

	printf("\n*****\n\n");
	
	if(motd) {
		struct stat sb;
		if(fstat(fileno(motd), &sb) < 0) {
			printf("stat() failed with code %ld", errno);
			return 1;
		}
		size_t l = sb.st_size;
		char* buffer = malloc(l + 1);
		fread(buffer, 1, l, motd);
		buffer[l] = '\0'; 
		printf("%s", buffer);
	}

	printf("\n*****\n\n");

	return 0;
}
