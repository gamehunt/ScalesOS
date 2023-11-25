cmake_minimum_required(VERSION 3.23)

project(Scales-Lockscreen VERSION 1.0.0)

add_executable(lockscreen src/main.c)
target_link_libraries(lockscreen PRIVATE compose widgets jpeg auth cfg fb)

install(TARGETS lockscreen DESTINATION bin/)
