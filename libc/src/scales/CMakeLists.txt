set(C_SOURCES
	prctl.c
	memory.c
)

target_sources(c PRIVATE ${C_SOURCES})
target_sources(k PRIVATE memory.c)
