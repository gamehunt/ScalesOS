cmake_minimum_required(VERSION 3.23)

project(Scales-Libinput C)

add_library(input SHARED 
	src/kbd.c
	src/mouse.c
)
set_property(TARGET input PROPERTY POSITION_INDEPENDENT_CODE 1)

target_include_directories(input PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/)
set_target_properties(input PROPERTIES PUBLIC_HEADER "include/kbd.h;include/mouse.h;include/keys.h")

install(TARGETS input
	PUBLIC_HEADER DESTINATION include/input/
)

install(TARGETS input
	LIBRARY DESTINATION lib/
)
