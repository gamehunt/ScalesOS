set(SOURCES 
    fork.c
    getpid.c
    exec.c
	usleep.c
	sleep.c
)

target_sources(c PRIVATE ${SOURCES})
