set(SOURCES 
    udivmoddi4.c
)

target_sources(k PRIVATE ${SOURCES})

add_subdirectory(stdio)
add_subdirectory(string)
add_subdirectory(stdlib)