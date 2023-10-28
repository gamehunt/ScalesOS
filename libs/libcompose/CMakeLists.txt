cmake_minimum_required(VERSION 3.23)

project(Scales-Libcompose C)

add_library(compose SHARED src/compose.c)
set_property(TARGET compose PROPERTY POSITION_INDEPENDENT_CODE 1)
target_include_directories(compose PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/)
set_target_properties(compose PROPERTIES PUBLIC_HEADER "include/compose.h")

install(TARGETS compose
	PUBLIC_HEADER DESTINATION include/
)

install(TARGETS compose
	LIBRARY DESTINATION lib/
)
