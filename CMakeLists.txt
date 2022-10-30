cmake_minimum_required(VERSION 3.23)

project(Scales)

set(RAMDISK_FOLDER        ${PROJECT_SOURCE_DIR}/build/ramdisk)
set(SHARED_HEADERS_FOLDER ${PROJECT_SOURCE_DIR}/shared)
set(LIBC_HEADERS_FOLDER   ${PROJECT_SOURCE_DIR}/libc/include)

add_subdirectory(libc)
add_subdirectory(kernel)
add_subdirectory(modules)