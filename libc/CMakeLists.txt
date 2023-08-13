cmake_minimum_required(VERSION 3.23)

project(Scales-LibC C ASM_NASM)

add_compile_definitions(PRINTF_DISABLE_SUPPORT_PTRDIFF_T)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/ ${CMAKE_CURRENT_SOURCE_DIR}/../kernel/include)

set(CMAKE_C_FLAGS "-ffreestanding -O0 -Wall -Wextra -mno-red-zone")

add_library(k STATIC)
add_library(c STATIC)

target_compile_definitions(k PRIVATE __LIBK)
target_compile_definitions(c PRIVATE __LIBC)

add_subdirectory(src)

install(TARGETS k ARCHIVE  DESTINATION lib/)
install(TARGETS c ARCHIVE  DESTINATION lib/)
install(FILES crt0.o DESTINATION lib/)
install(DIRECTORY include/ DESTINATION include/)

add_custom_command(
    TARGET c POST_BUILD
    COMMAND nasm -felf32 ${CMAKE_CURRENT_SOURCE_DIR}/src/crt0.asm -o crt0.o
    COMMAND cp crt0.o ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM
)
