#include <sys/syscall.h>

int main(){
    if(__syscall(1, 0, 0, 0, 0, 0)){
        __syscall(0, "IN PARENT", 0, 0, 0, 0);
    }else{
        __syscall(0, "IN FORK", 0, 0, 0, 0);
    }
    while(1);
    return 0;
}