cmake_minimum_required(VERSION 3.23)

project(Scales-Libpng C)

add_library(png OBJECT src/png.c)
set_property(TARGET png PROPERTY POSITION_INDEPENDENT_CODE 1)

target_include_directories(png PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/)
set_target_properties(png PROPERTIES PUBLIC_HEADER "include/png.h")

add_library(pngst STATIC $<TARGET_OBJECTS:png>)
add_library(pngso SHARED $<TARGET_OBJECTS:png>)

set_target_properties(pngst PROPERTIES OUTPUT_NAME png)
set_target_properties(pngso PROPERTIES OUTPUT_NAME png)

install(TARGETS png
	PUBLIC_HEADER DESTINATION include/
)

install(TARGETS pngst pngso
	LIBRARY DESTINATION lib/
)

install(TARGETS pngso
	LIBRARY DESTINATION ramdisk/lib/
)
