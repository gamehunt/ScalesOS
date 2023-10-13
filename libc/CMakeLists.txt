cmake_minimum_required(VERSION 3.23)

project(Scales-LibC C ASM_NASM)

add_compile_definitions(PRINTF_DISABLE_SUPPORT_PTRDIFF_T)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/ ${CMAKE_CURRENT_SOURCE_DIR}/../kernel/include)

set(CMAKE_C_FLAGS "-ffreestanding -O0 -Wall -Wextra -mno-red-zone")

add_library(k STATIC)

add_library(c OBJECT)
set_property(TARGET c PROPERTY POSITION_INDEPENDENT_CODE 1)

add_library(cst STATIC $<TARGET_OBJECTS:c>)
add_library(cso SHARED $<TARGET_OBJECTS:c>)

target_compile_definitions(k   PRIVATE __LIBK)
target_compile_definitions(cst PRIVATE __LIBC)
target_compile_definitions(cso PRIVATE __LIBC)

set_target_properties(cst PROPERTIES OUTPUT_NAME c)
set_target_properties(cso PROPERTIES OUTPUT_NAME c)

add_subdirectory(src)

install(TARGETS k ARCHIVE  DESTINATION lib/)
install(TARGETS cst ARCHIVE  DESTINATION lib/)
install(TARGETS cso ARCHIVE  DESTINATION lib/)
install(FILES crt0.o crti.o crtn.o DESTINATION lib/)
install(DIRECTORY include/ DESTINATION include/)

add_custom_command(
    TARGET cst POST_BUILD
    COMMAND nasm -felf32 ${CMAKE_CURRENT_SOURCE_DIR}/src/crt0.asm -o crt0.o
    COMMAND nasm -felf32 ${CMAKE_CURRENT_SOURCE_DIR}/src/crti.asm -o crti.o
    COMMAND nasm -felf32 ${CMAKE_CURRENT_SOURCE_DIR}/src/crtn.asm -o crtn.o
    COMMAND cp crt0.o ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND cp crti.o ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND cp crtn.o ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM
)

add_custom_command(
    TARGET cso POST_BUILD
    COMMAND nasm -felf32 ${CMAKE_CURRENT_SOURCE_DIR}/src/crt0.asm -o crt0.o
    COMMAND nasm -felf32 ${CMAKE_CURRENT_SOURCE_DIR}/src/crti.asm -o crti.o
    COMMAND nasm -felf32 ${CMAKE_CURRENT_SOURCE_DIR}/src/crtn.asm -o crtn.o
    COMMAND cp crt0.o ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND cp crti.o ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND cp crtn.o ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM
)
