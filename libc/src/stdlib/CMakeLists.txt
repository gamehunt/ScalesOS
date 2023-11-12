set(SHARED_SOURCES 
	alloc.c
    abort.c
    abs.c
    atoi.c
)

set(C_SOURCES
    env.c
    atexit.c
    exit.c
	system.c
)

target_sources(k PRIVATE ${SHARED_SOURCES})
target_sources(c PRIVATE ${SHARED_SOURCES} ${C_SOURCES})
