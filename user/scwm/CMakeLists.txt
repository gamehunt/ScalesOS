cmake_minimum_required(VERSION 3.23)

project(Scales-Scwm VERSION 1.0.0)

add_executable(scwm
	src/main.c
)
target_link_libraries(scwm PRIVATE compose)

install(TARGETS scwm DESTINATION bin/)
