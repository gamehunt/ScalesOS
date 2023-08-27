cmake_minimum_required(VERSION 3.23)

project(Scales-Login VERSION 1.0.0)

add_executable(login src/main.c)

install(TARGETS login DESTINATION bin/)

