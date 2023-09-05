cmake_minimum_required(VERSION 3.23)

project(Scales-Stty VERSION 1.0.0)

add_executable(stty src/main.c)

install(TARGETS stty DESTINATION bin/)
