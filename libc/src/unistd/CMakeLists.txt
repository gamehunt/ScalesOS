set(SOURCES 
    fork.c
    getpid.c
    exec.c
	usleep.c
	sleep.c
	dup.c
	chdir.c
	getcwd.c
	getuid.c
	mmap.c
	fs.c
	clone.c
)

target_sources(c PRIVATE ${SOURCES})
