cmake_minimum_required(VERSION 3.23)

project(Scales-Motd VERSION 1.0.0)

add_executable(motd src/main.c)

install(TARGETS motd DESTINATION bin/)
