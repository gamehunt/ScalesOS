cmake_minimum_required(VERSION 3.23)

project(Scales-Libwidgets C)

add_library(widgets SHARED 
	src/widget.c
	src/label.c
	src/input.c
	src/button.c
	src/win.c
)
set_property(TARGET widgets PROPERTY POSITION_INDEPENDENT_CODE 1)

target_include_directories(widgets PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/)
set_target_properties(widgets PROPERTIES PUBLIC_HEADER 
	"include/widget.h;include/label.h;include/input.h;include/button.h;include/win.h;"
)

target_link_libraries(widgets PRIVATE compose types)

install(TARGETS widgets
	PUBLIC_HEADER DESTINATION include/widgets/
)

install(TARGETS widgets
	LIBRARY DESTINATION lib/
)
