#ifndef __K_UTIL_PERF_H
#define __K_UTIL_PERF_H

#include <stdint.h>

typedef struct {
	const char* name;
	uint64_t    start;
} perf_t;

void k_util_perf_start(perf_t* i, const char* name);
void k_util_perf_end(perf_t* i);

#endif
