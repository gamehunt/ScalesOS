set(SOURCES 
    memset.c
    memcpy.c
    memmove.c
    memcmp.c
    strcpy.c
    strlen.c
    strtok.c
    strcmp.c
    strcat.c
	strncat.c
    strdup.c
    strchr.c
    strchrnul.c
    strcspn.c
    strncmp.c
	strtol.c
	strncpy.c
	strstr.c
	strnstr.c
)

target_sources(k PRIVATE ${SOURCES})
target_sources(c PRIVATE ${SOURCES})
