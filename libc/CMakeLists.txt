cmake_minimum_required(VERSION 3.23)

project(Scales-LibC C)

add_compile_definitions(PRINTF_DISABLE_SUPPORT_PTRDIFF_T)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/)

add_library(k STATIC)
#add_library(c STATIC)

target_compile_options(k PRIVATE -ffreestanding -O0 -Wall -Wextra -mno-red-zone)

add_subdirectory(src)

install(TARGETS k ARCHIVE  DESTINATION lib/)
install(DIRECTORY include/ DESTINATION include/)