cmake_minimum_required(VERSION 3.23)

project(Scales-Init VERSION 1.0.0)

add_executable(init src/main.c)

install(TARGETS init DESTINATION bin/)
install(TARGETS init DESTINATION ramdisk/bin/)
