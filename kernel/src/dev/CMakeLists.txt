set(SOURCES 
    serial.c
)

target_sources(kernel.bin PRIVATE ${SOURCES})
