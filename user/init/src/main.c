#include <stdint.h>
#include <sys/syscall.h>

int main(){
	for(int i = 0; i < 10; i++) {
		if(!__syscall(1, 0, 0, 0, 0, 0)) {
			__syscall(0, "FORK!", 0, 0, 0 ,0);
			while(1);
		}
	}
	while(1);
    return 0;
}
