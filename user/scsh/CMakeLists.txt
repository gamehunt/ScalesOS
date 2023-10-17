cmake_minimum_required(VERSION 3.23)

project(Scales-Scsh VERSION 1.0.0)

add_executable(scsh src/main.c)

install(TARGETS scsh DESTINATION bin/)
install(TARGETS scsh DESTINATION ramdisk/bin)

