set(SOURCES 
    fork.c
    getpid.c
    exec.c
)

target_sources(c PRIVATE ${SOURCES})