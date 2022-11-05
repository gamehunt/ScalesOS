#include <stdint.h>

extern uint64_t __udivmoddi4(uint64_t num, uint64_t den, uint64_t* rem_p);

uint64_t __umoddi3(uint64_t a, uint64_t b)
{
    uint64_t r;
    __udivmoddi4(a, b, &r);
    return r;
}