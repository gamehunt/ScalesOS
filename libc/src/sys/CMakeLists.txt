set(SOURCES 
    syscall.asm
    syscall.c
	wait.c
	waitpid.c
)

target_sources(c PRIVATE ${SOURCES})
