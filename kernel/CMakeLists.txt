cmake_minimum_required(VERSION 3.23)

project(Scales-Kernel VERSION 1.0.0)

enable_language(C ASM_NASM)

add_definitions(-DKERNEL_VERSION="${CMAKE_PROJECT_VERSION}" -DDEBUG)

set(CMAKE_C_FLAGS "-ffreestanding -O0 -Wall -Wextra")
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_LINKER> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

add_executable(kernel.bin)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/ ${SYSROOT_FOLDER}/include)
target_link_options(kernel.bin PRIVATE -T${CMAKE_CURRENT_SOURCE_DIR}/link.ld -ffreestanding -O2 -nostdlib)
target_link_libraries(kernel.bin PRIVATE gcc k)

add_subdirectory(src)

add_custom_command(
    TARGET  kernel.bin POST_BUILD
    COMMAND ${SCRIPTS_FOLDER}/create_kernel_symtable.sh > symtable.h
    COMMAND cp -a ${CMAKE_CURRENT_SOURCE_DIR}/include/.  ${SYSROOT_FOLDER}/include/kernel
    COMMAND cp symtable.h ${SYSROOT_FOLDER}/include/kernel
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)