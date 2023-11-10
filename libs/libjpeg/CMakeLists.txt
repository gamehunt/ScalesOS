cmake_minimum_required(VERSION 3.23)

project(Scales-Libjpeg C)

add_library(jpeg SHARED src/jpeg.c)

set_property(TARGET jpeg PROPERTY POSITION_INDEPENDENT_CODE 1)
set_target_properties(jpeg PROPERTIES PUBLIC_HEADER "include/jpeg.h")

target_include_directories(jpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/)

install(TARGETS jpeg
	PUBLIC_HEADER DESTINATION include/
)

install(TARGETS jpeg
	LIBRARY DESTINATION lib/
)

install(TARGETS jpeg
	LIBRARY DESTINATION ramdisk/lib/
)
