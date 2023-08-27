cmake_minimum_required(VERSION 3.23)
include(ExternalProject)

project(Scales)

set(SCRIPTS_FOLDER ${PROJECT_SOURCE_DIR}/scripts)
set(RAMDISK_FOLDER ${PROJECT_SOURCE_DIR}/build/ramdisk)
set(SYSROOT_FOLDER ${PROJECT_SOURCE_DIR}/sysroot)

ExternalProject_Add(
    shared
    SOURCE_DIR  ${PROJECT_SOURCE_DIR}/shared/
    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX=${SYSROOT_FOLDER}
)

ExternalProject_Add(
    libc
    SOURCE_DIR  ${PROJECT_SOURCE_DIR}/libc/
    BINARY_DIR  ${PROJECT_SOURCE_DIR}/build/libc/
    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX=${SYSROOT_FOLDER} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=${PROJECT_SOURCE_DIR}/toolchain/i686_scales_toolchain.cmake
)

ExternalProject_Add(
    kernel
    SOURCE_DIR  ${PROJECT_SOURCE_DIR}/kernel/
    BINARY_DIR  ${PROJECT_SOURCE_DIR}/build/kernel/
    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX=${SYSROOT_FOLDER} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=${PROJECT_SOURCE_DIR}/toolchain/i686_scales_toolchain.cmake
)

ExternalProject_Add(
    modules
    SOURCE_DIR  ${PROJECT_SOURCE_DIR}/modules/
    BINARY_DIR  ${PROJECT_SOURCE_DIR}/build/modules/
    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX=${SYSROOT_FOLDER} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=${PROJECT_SOURCE_DIR}/toolchain/i686_scales_toolchain.cmake
)

ExternalProject_Add(
    userspace
    SOURCE_DIR  ${PROJECT_SOURCE_DIR}/user/
    BINARY_DIR  ${PROJECT_SOURCE_DIR}/build/user/
    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX=${SYSROOT_FOLDER} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=${PROJECT_SOURCE_DIR}/toolchain/i686_scales_toolchain.cmake
)

add_custom_target(
    compiler_hints
    COMMAND bash    ${SCRIPTS_FOLDER}/generate_compiler_hints.sh
    COMMAND python  ${SCRIPTS_FOLDER}/postprocess_hints.py
    VERBATIM
)

add_custom_target(
    ramdisk
	COMMAND mkdir -p ramdisk/dev
	COMMAND mkdir -p ramdisk/etc
    COMMAND tar -cvf scales.initrd -C ramdisk .
    COMMAND mv scales.initrd bin/scales.initrd
    WORKING_DIRECTORY ${SYSROOT_FOLDER}
    VERBATIM
)

install(FILES     ${SYSROOT_FOLDER}/bin/kernel.bin    DESTINATION /mnt/boot)
install(FILES     ${SYSROOT_FOLDER}/bin/scales.initrd DESTINATION /mnt/boot)
install(DIRECTORY ${SYSROOT_FOLDER}/include           DESTINATION /mnt)
install(DIRECTORY ${SYSROOT_FOLDER}/lib               DESTINATION /mnt)
install(DIRECTORY ${SYSROOT_FOLDER}/etc               DESTINATION /mnt)
install(DIRECTORY ${SYSROOT_FOLDER}/bin               DESTINATION /mnt)
install(DIRECTORY ${SYSROOT_FOLDER}/dev               DESTINATION /mnt)
