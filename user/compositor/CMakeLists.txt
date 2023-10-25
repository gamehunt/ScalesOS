cmake_minimum_required(VERSION 3.23)

project(Scales-Compositor VERSION 1.0.0)

add_executable(compositor
	src/main.c
)
target_link_libraries(compositor PRIVATE fb)

install(TARGETS compositor DESTINATION bin/)
