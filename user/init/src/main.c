#include <sys/syscall.h>

int main(){
    int i = 0;
    while(1){
        i++;
        __syscall(0, i, 2, 3, 4 ,5);
    }
    return 0;
}