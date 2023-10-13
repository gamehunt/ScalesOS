cmake_minimum_required(VERSION 3.23)

project(Scales-Ld VERSION 1.0.0)

add_executable(ld.so ld.c)
target_link_libraries(ld.so -static)

install(TARGETS ld.so DESTINATION lib/)
