set(SHARED_SOURCES 
    udivmoddi4.c
    udivdi3.c
    umoddi3.c
)

target_sources(k PRIVATE ${SHARED_SOURCES})
target_sources(c PRIVATE ${SHARED_SOURCES} libc.c tls.c)

add_subdirectory(arch/${CMAKE_SYSTEM_PROCESSOR})
add_subdirectory(stdio)
add_subdirectory(string)
add_subdirectory(stdlib)
add_subdirectory(errno)
add_subdirectory(unistd)
add_subdirectory(sys)
add_subdirectory(time)
add_subdirectory(signal)
add_subdirectory(dirent)
add_subdirectory(ctype)
add_subdirectory(pwd)
add_subdirectory(math)
add_subdirectory(scales)
add_subdirectory(fcntl)

