cmake_minimum_required(VERSION 3.23)

project(Scales-Libtga C)

add_library(tga SHARED src/tga.c)
set_property(TARGET tga PROPERTY POSITION_INDEPENDENT_CODE 1)

target_include_directories(tga PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/)
set_target_properties(tga PROPERTIES PUBLIC_HEADER "include/tga.h")

install(TARGETS tga
	PUBLIC_HEADER DESTINATION include/
)

install(TARGETS tga
	LIBRARY DESTINATION lib/
)

install(TARGETS tga
	LIBRARY DESTINATION ramdisk/lib/
)
