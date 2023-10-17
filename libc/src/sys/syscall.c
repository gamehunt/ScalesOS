#include <sys/syscall.h>

extern uint32_t __syscall(uint32_t num, uint32_t a, uint32_t b, uint32_t c, uint32_t e, uint32_t f);

uint32_t syscall(uint32_t num, uint32_t a, uint32_t b, uint32_t c, uint32_t e, uint32_t f) {
	return __syscall(num, a, b, c, e, f);
}
