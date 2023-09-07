set(SOURCES 
    serial.c
    pit.c
    pci.c
    timer.asm
    timer.c
    speaker.c
    fb.c
    fpu.asm
    fpu.c
    cmos.c
    rtc.c
    acpi.c
    ps2.c
	null.c
	tty.c
	random.asm
	random.c
	vt.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})
