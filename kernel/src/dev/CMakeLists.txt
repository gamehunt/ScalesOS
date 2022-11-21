set(SOURCES 
    serial.c
    pit.c
    pci.c
    timer.asm
    timer.c
    speaker.c
    fb.c
    console.c
    fpu.asm
    fpu.c
    cmos.c
    rtc.c
    acpi.c
    ps2.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})
