cmake_minimum_required(VERSION 3.23)

project(Scales-Libfb C)

add_library(fb SHARED src/fb.c)
set_property(TARGET fb PROPERTY POSITION_INDEPENDENT_CODE 1)
target_include_directories(fb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/)
set_target_properties(fb PROPERTIES PUBLIC_HEADER "include/fb.h;include/fb_colors.h")

install(TARGETS fb
	PUBLIC_HEADER DESTINATION include/
)

install(TARGETS fb
	LIBRARY DESTINATION lib/
)

install(TARGETS fb
	LIBRARY DESTINATION ramdisk/lib/
)

install(DIRECTORY fonts/ DESTINATION res/fonts)
install(DIRECTORY fonts/ DESTINATION ramdisk/res/fonts)
