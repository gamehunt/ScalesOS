#include <unistd.h>
#include <sys/syscall.h>

pid_t getpid(){
    return __sys_getpid();
}
