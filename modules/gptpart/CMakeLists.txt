cmake_minimum_required(VERSION 3.23)

project(Scales-Gptpart-Module VERSION 1.0.0)

enable_language(C ASM_NASM)

set(CMAKE_C_FLAGS "-ffreestanding -O0 -Wall -Wextra")
set(CMAKE_C_STANDARD 11)

set(TARGET_FILE gptpart.scmod)

include_directories(${CMAKE_INSTALL_PREFIX}/include)
include_directories(${CMAKE_INSTALL_PREFIX}/include/kernel)

add_library(gptpart_module_objs OBJECT main.c)

add_custom_target(
    gptpart_module ALL
    COMMAND ${CMAKE_LINKER} -r $<TARGET_OBJECTS:gptpart_module_objs> -ffreestanding -O0 -nostdlib -o ${TARGET_FILE} -L${CMAKE_INSTALL_PREFIX}/lib -lk -lgcc
    DEPENDS $<TARGET_OBJECTS:gptpart_module_objs>
    VERBATIM
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_FILE} DESTINATION bin/modules)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_FILE} DESTINATION ramdisk/modules)
