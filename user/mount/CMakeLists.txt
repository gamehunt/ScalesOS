cmake_minimum_required(VERSION 3.23)

project(Scales-Mount VERSION 1.0.0)

add_executable(mount src/main.c)

install(TARGETS mount DESTINATION bin/)
install(TARGETS mount DESTINATION ramdisk/bin/)
