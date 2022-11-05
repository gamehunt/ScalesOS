#include <stdint.h>

extern uint64_t __udivmoddi4(uint64_t num, uint64_t den, uint64_t* rem_p);

uint64_t __udivdi3(uint64_t a, uint64_t b)
{
    return __udivmoddi4(a, b, (uint64_t *)0);
}