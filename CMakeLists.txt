cmake_minimum_required(VERSION 3.23)

project(Scales)

set(SHARED_HEADERS_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/shared)
set(LIBC_HEADERS_FOLDER   ${CMAKE_CURRENT_SOURCE_DIR}/libc/include)

add_subdirectory(libc)
add_subdirectory(kernel)