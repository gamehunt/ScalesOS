cmake_minimum_required(VERSION 3.23)

project(Scales-Kernel VERSION 1.0.0)

enable_language(C ASM_NASM)

add_definitions(-DKERNEL_VERSION="${CMAKE_PROJECT_VERSION}" -DVIRTUAL_BASE=0xC0000000 -DDEBUG)

set(CMAKE_C_FLAGS "-ffreestanding -O0 -Wall -Wextra")
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_LINKER> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

add_library(kernel.bin OBJECT)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/ ${SYSROOT_FOLDER}/include)
add_subdirectory(src)

add_executable(kernel.bin.tmp $<TARGET_OBJECTS:kernel.bin>)
target_link_options(kernel.bin.tmp PRIVATE -T${CMAKE_CURRENT_SOURCE_DIR}/link.ld -ffreestanding -O0 -nostdlib)
target_link_libraries(kernel.bin.tmp PRIVATE gcc k)
add_dependencies(kernel.bin.tmp kernel.bin)

add_custom_target(
    symtable ALL
    COMMAND readelf -s --wide kernel.bin.tmp | python ${CMAKE_CURRENT_SOURCE_DIR}/generate_symtable.py > symtable.asm
    COMMAND nasm -felf32 symtable.asm
)
add_dependencies(symtable kernel.bin.tmp)

add_executable(kernel.bin.final $<TARGET_OBJECTS:kernel.bin>)
set_target_properties(kernel.bin.final PROPERTIES OUTPUT_NAME kernel.bin)
target_link_options(kernel.bin.final PRIVATE -T${CMAKE_CURRENT_SOURCE_DIR}/link.ld -ffreestanding -O0 -nostdlib symtable.o)
target_link_libraries(kernel.bin.final PRIVATE gcc k)
add_dependencies(kernel.bin.final symtable)

install(TARGETS kernel.bin.final RUNTIME DESTINATION /mnt/boot)
install(DIRECTORY include/ DESTINATION ${SYSROOT_FOLDER}/include/kernel)