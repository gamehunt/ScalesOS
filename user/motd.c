#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
	FILE* motd = fopen("/etc/motd", "r");

	printf("\n*****\n\n");
	
	if(motd) {
		size_t l = fseek(motd, 0, SEEK_END);
		fseek(motd, 0, SEEK_SET);
		char* buffer = malloc(l + 1);
		fread(buffer, 1, l, motd);
		buffer[l] = '\0'; 
		printf("%s", buffer);
	}

	printf("\n*****\n");

	return 0;
}
