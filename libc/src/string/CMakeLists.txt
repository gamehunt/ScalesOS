set(SOURCES 
    memset.c
    memcpy.c
    memmove.c
    strcpy.c
    strlen.c
    strtok.c
    strcmp.c
    strcat.c
)

target_sources(k PRIVATE ${SOURCES})