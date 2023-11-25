cmake_minimum_required(VERSION 3.23)

project(Scales-Ext2-Module VERSION 1.0.0)

enable_language(C ASM_NASM)

add_definitions(
	-D__MODULE
)

set(CMAKE_C_FLAGS "-ffreestanding -O0 -Wall -Wextra")
set(CMAKE_C_STANDARD 11)

set(TARGET_FILE ext2.scmod)

include_directories(${CMAKE_INSTALL_PREFIX}/include)
include_directories(${CMAKE_INSTALL_PREFIX}/include/kernel)

add_library(ext2_module_objs OBJECT main.c)

add_custom_target(
    ext2_module ALL
    COMMAND ${CMAKE_LINKER} -r $<TARGET_OBJECTS:ext2_module_objs> -ffreestanding -O0 -nostdlib -o ${TARGET_FILE} -L${CMAKE_INSTALL_PREFIX}/lib -lgcc
    DEPENDS $<TARGET_OBJECTS:ext2_module_objs>
    VERBATIM
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_FILE} DESTINATION ramdisk/modules)
