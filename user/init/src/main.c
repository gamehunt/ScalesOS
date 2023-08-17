#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char** argv){
	printf("Hello, world! %d 0x%x\r\n", argc, argv);
	int* heaped = malloc(sizeof(int));
	*heaped = 0xAABBCCDD;
	printf("0x%x 0x%x\r\n", (uint32_t) heaped, *heaped);
	free(heaped);
	if(fork()) {
		int status;
		printf("Sleeping...\r\n");
		wait(&status);
		printf("Woke up.\r\n");
		while(1);
	} else{
		printf("Forked!\r\n");
		heaped  = malloc(16 * 1024 * 1024);
		*heaped = 12345; 
		printf("0x%x %d\r\n", heaped, *heaped);
		for(int i = 0; i < 25; i++) {
			printf("%d\r\n", i);
		}
	}
    return 0;
}
