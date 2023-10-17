cmake_minimum_required(VERSION 3.23)

project(Scales-Ld VERSION 1.0.0 LANGUAGES C ASM_NASM)

set(LINKER_SCRIPT "link.ld")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${CMAKE_CURRENT_SOURCE_DIR}/${LINKER_SCRIPT}")

add_executable(ld.so ld.c wrapper.asm)
target_link_libraries(ld.so -static)

install(TARGETS ld.so DESTINATION lib/)
install(TARGETS ld.so DESTINATION ramdisk/lib/)
