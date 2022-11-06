set(SHARED_SOURCES 
    malloc.c
    free.c
    abort.c
    abs.c
    atoi.c
)

set(C_SOURCES
    getenv.c
    atexit.c
    exit.c
)

target_sources(k PRIVATE ${SHARED_SOURCES})
target_sources(c PRIVATE ${SHARED_SOURCES} ${C_SOURCES})