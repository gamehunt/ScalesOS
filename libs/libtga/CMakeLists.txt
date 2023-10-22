cmake_minimum_required(VERSION 3.23)

project(Scales-Libtga C)

add_library(tga OBJECT src/tga.c)
set_property(TARGET tga PROPERTY POSITION_INDEPENDENT_CODE 1)

target_include_directories(tga PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/)
set_target_properties(tga PROPERTIES PUBLIC_HEADER "include/tga.h")

add_library(tgast STATIC $<TARGET_OBJECTS:tga>)
add_library(tgaso SHARED $<TARGET_OBJECTS:tga>)

set_target_properties(tgast PROPERTIES OUTPUT_NAME tga)
set_target_properties(tgaso PROPERTIES OUTPUT_NAME tga)

install(TARGETS tga
	PUBLIC_HEADER DESTINATION include/
)

install(TARGETS tgast tgaso
	LIBRARY DESTINATION lib/
)

install(TARGETS tgaso
	LIBRARY DESTINATION ramdisk/lib/
)
