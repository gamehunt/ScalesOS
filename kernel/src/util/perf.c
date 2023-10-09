#include "util/perf.h"
#include "dev/timer.h"
#include "util/log.h"

void k_util_perf_start(perf_t* i, const char* name) {
	i->name  = name;	
	i->start = k_dev_timer_read_tsc();
}

void k_util_perf_end(perf_t* i) {
	uint64_t current = k_dev_timer_read_tsc();
	uint64_t delta   = current - i->start;
	uint64_t ticks   = delta / k_dev_timer_get_core_speed();
	uint64_t seconds      = ticks / 1000000;
	uint64_t milliseconds = (ticks % 1000000) / 1000;
	uint64_t microseconds = (ticks % 1000000) % 1000;

	k_info("%s took %lld:%lld:%lld", i->name, seconds, milliseconds, microseconds);
}
