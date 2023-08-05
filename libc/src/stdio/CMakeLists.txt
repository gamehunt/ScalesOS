set(SOURCES 
    printf.c
)

set(C_SOURCES stdio.c)

target_sources(k PRIVATE ${SOURCES})
target_sources(c PRIVATE ${SOURCES} ${C_SOURCES})
