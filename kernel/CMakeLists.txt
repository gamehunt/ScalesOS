cmake_minimum_required(VERSION 3.23)

project(Scales-Kernel VERSION 1.0.0)

enable_language(C ASM_NASM)

add_definitions(-D__KERNEL -DKERNEL_VERSION="${CMAKE_PROJECT_VERSION}" -DVIRTUAL_BASE=0xC0000000 -DDEBUG)

set(CMAKE_C_FLAGS "-ffreestanding -O0 -Wall -Wextra -mno-red-zone")
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_LINKER> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_EXE_LINKER_FLAGS  "-ffreestanding -O0 -nostdlib -mno-red-zone")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/)
link_libraries(gcc k)

add_library(kernel.bin OBJECT)
add_subdirectory(src)

add_custom_command(
    OUTPUT font.o
    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/font.psf .
    COMMAND objcopy -O elf32-i386 -B i386 -I binary font.psf font.o
)

add_executable(kernel.bin.tmp $<TARGET_OBJECTS:kernel.bin> font.o)
target_link_options(kernel.bin.tmp PRIVATE -T${CMAKE_CURRENT_SOURCE_DIR}/link.ld)
add_dependencies(kernel.bin.tmp kernel.bin)

add_custom_command(
    OUTPUT symtable.o
    COMMAND readelf -s --wide kernel.bin.tmp | python ${CMAKE_CURRENT_SOURCE_DIR}/generate_symtable.py > symtable.asm
    COMMAND nasm -felf32 symtable.asm
)

add_executable(kernel.bin.final $<TARGET_OBJECTS:kernel.bin> font.o symtable.o)
set_target_properties(kernel.bin.final PROPERTIES OUTPUT_NAME kernel.bin)
target_link_options(kernel.bin.final PRIVATE -T${CMAKE_CURRENT_SOURCE_DIR}/link.ld)
add_dependencies(kernel.bin.final kernel.bin.tmp)

install(TARGETS kernel.bin.final RUNTIME DESTINATION bin/)
install(DIRECTORY include/ DESTINATION include/kernel)