#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(){
	printf("Hello, world!\r\n");
	int* heaped = malloc(sizeof(int));
	*heaped = 0xAABBCCDD;
	printf("0x%x 0x%x\r\n", (uint32_t) heaped, *heaped);
	free(heaped);
	if(fork()) {
		while(1);
	} else{
		printf("Forked!\r\n");
		heaped = 0x0;
		printf("0x%x", *heaped);
	}
    return 0;
}
