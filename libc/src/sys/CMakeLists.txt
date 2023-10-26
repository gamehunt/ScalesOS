set(SOURCES 
    syscall.asm
    syscall.c
	wait.c
	waitpid.c
	times.c
	time.c
	mount.c
	stat.c
	ioctl.c
	tty.c
	mod.c
	utsname.c
	reboot.c
	heap.c
	socket.c
	select.c
	poll.c
)

target_sources(c PRIVATE ${SOURCES})
