cmake_minimum_required(VERSION 3.23)

project(Scales-Libtypes C ASM_NASM)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/)

add_library(ktypes STATIC)
add_library(types  STATIC)
add_library(stypes SHARED)
set_property(TARGET stypes PROPERTY POSITION_INDEPENDENT_CODE 1)
set_target_properties(stypes PROPERTIES OUTPUT_NAME types)

add_subdirectory(src)

install(TARGETS ktypes ARCHIVE DESTINATION lib/)
install(TARGETS types  ARCHIVE DESTINATION lib/)
install(TARGETS stypes LIBRARY DESTINATION lib/)
install(TARGETS stypes LIBRARY DESTINATION ramdisk/lib/)
install(DIRECTORY include/ DESTINATION include/types/)

