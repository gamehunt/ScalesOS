set(SOURCES 
    serial.c
    pit.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})
