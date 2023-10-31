cmake_minimum_required(VERSION 3.23)

project(Scales-Libpthread C)

set(SOURCES 
    src/pthread.c
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/)

add_library(pthread OBJECT ${SOURCES})
set_property(TARGET pthread PROPERTY POSITION_INDEPENDENT_CODE 1)

add_library(pthread_sh SHARED $<TARGET_OBJECTS:pthread>)
add_library(pthread_st STATIC $<TARGET_OBJECTS:pthread>)

set_target_properties(pthread PROPERTIES PUBLIC_HEADER "include/pthread.h")

set_target_properties(pthread_sh PROPERTIES OUTPUT_NAME pthread)
set_target_properties(pthread_st PROPERTIES OUTPUT_NAME pthread)

install(TARGETS pthread
	PUBLIC_HEADER DESTINATION include/
)

install(TARGETS pthread_sh
	LIBRARY DESTINATION lib/
)

install(TARGETS pthread_sh
	LIBRARY DESTINATION ramdisk/lib/
)

install(TARGETS pthread_st
	ARCHIVE DESTINATION lib/
)

