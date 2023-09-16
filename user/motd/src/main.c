#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
	FILE* motd = fopen("/etc/motd", "r");

	printf("*****\n");
	
	if(motd) {
		size_t l = fseek(motd, 0, SEEK_END);
		fseek(motd, 0, SEEK_SET);
		char* buffer = malloc(l);
		fread(buffer, 1, l, motd);
		printf("%s", buffer);
	}

	printf("\n*****\n");

	return 0;
}
