set(SOURCES 
    syscall.asm
    syscall.c
	wait.c
	waitpid.c
	times.c
	time.c
)

target_sources(c PRIVATE ${SOURCES})
