cmake_minimum_required(VERSION 3.23)

project(Scales-Umount VERSION 1.0.0)

add_executable(umount src/main.c)

install(TARGETS umount DESTINATION bin/)
install(TARGETS umount DESTINATION ramdisk/bin/)
