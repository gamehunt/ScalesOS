cmake_minimum_required(VERSION 3.23)

project(Scales-Libtypes C ASM_NASM)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/)

add_library(ktypes STATIC)
add_library(types  STATIC)

add_subdirectory(src)

install(TARGETS ktypes ARCHIVE DESTINATION lib/)
install(TARGETS types  ARCHIVE DESTINATION lib/)
install(DIRECTORY include/ DESTINATION include/types/)

