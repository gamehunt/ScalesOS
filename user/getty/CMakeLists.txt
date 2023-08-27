cmake_minimum_required(VERSION 3.23)

project(Scales-Getty VERSION 1.0.0)

add_executable(getty src/main.c)

install(TARGETS getty DESTINATION bin/)
