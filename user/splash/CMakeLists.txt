cmake_minimum_required(VERSION 3.23)

project(Scales-Splash VERSION 1.0.0)

add_executable(splash src/main.c)

install(TARGETS splash DESTINATION bin/)
install(TARGETS splash DESTINATION ramdisk/bin/)
