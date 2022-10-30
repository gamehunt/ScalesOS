cmake_minimum_required(VERSION 3.23)

project(Scales)

set(SCRIPTS_FOLDER ${PROJECT_SOURCE_DIR}/scripts)
set(RAMDISK_FOLDER ${PROJECT_SOURCE_DIR}/build/ramdisk)
set(SYSROOT_FOLDER ${PROJECT_SOURCE_DIR}/sysroot)

add_subdirectory(libc)
add_subdirectory(kernel)
add_subdirectory(modules)