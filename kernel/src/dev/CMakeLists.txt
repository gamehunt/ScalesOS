set(SOURCES 
    serial.c
    pit.c
    pci.c
    timer.c
    speaker.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})
