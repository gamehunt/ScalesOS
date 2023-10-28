cmake_minimum_required(VERSION 3.23)

project(Scales-Libpng C)

add_library(png SHARED src/png.c)

set_property(TARGET png PROPERTY POSITION_INDEPENDENT_CODE 1)
set_target_properties(png PROPERTIES PUBLIC_HEADER "include/png.h")

target_include_directories(png PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/)

install(TARGETS png
	PUBLIC_HEADER DESTINATION include/
)

install(TARGETS png
	LIBRARY DESTINATION lib/
)

install(TARGETS png
	LIBRARY DESTINATION ramdisk/lib/
)
