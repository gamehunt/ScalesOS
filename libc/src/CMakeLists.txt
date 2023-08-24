set(SHARED_SOURCES 
    udivmoddi4.c
    udivdi3.c
    umoddi3.c
)

target_sources(k PRIVATE ${SHARED_SOURCES})
target_sources(c PRIVATE ${SHARED_SOURCES} libc.c)

add_subdirectory(stdio)
add_subdirectory(string)
add_subdirectory(stdlib)
add_subdirectory(unistd)
add_subdirectory(sys)
add_subdirectory(time)
add_subdirectory(signal)
add_subdirectory(dirent)
