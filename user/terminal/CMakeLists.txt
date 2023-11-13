cmake_minimum_required(VERSION 3.23)

project(Scales-Terminal VERSION 1.0.0)

add_executable(terminal src/main.c)
target_link_libraries(terminal PRIVATE compose widgets)

install(TARGETS terminal DESTINATION bin/)
