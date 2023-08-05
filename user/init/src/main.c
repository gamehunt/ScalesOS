#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(){
	printf("Hello, world!\r\n");
	if(fork()) {
		while(1);
	} else{
		printf("Forked!\r\n");
	}
    return 0;
}
