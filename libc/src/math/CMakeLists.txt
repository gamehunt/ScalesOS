set(C_SOURCES
	math.asm
	math.c
)

target_sources(k PRIVATE ${C_SOURCES})
target_sources(c PRIVATE ${C_SOURCES})
