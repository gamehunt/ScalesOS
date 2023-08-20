#include <time.h>

char* ctime(time_t timep) {
    return asctime(localtime(&timep));
}
