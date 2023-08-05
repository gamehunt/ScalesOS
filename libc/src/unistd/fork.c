#include "sys/syscall.h"
#include <unistd.h>

pid_t fork(){
    return __sys_fork();
}
