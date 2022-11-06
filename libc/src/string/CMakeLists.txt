set(SOURCES 
    memset.c
    memcpy.c
    memmove.c
    strcpy.c
    strlen.c
    strtok.c
    strcmp.c
    strcat.c
    strdup.c
    strchr.c
    strchrnul.c
    strcspn.c
    strncmp.c
)

target_sources(k PRIVATE ${SOURCES})
target_sources(c PRIVATE ${SOURCES})