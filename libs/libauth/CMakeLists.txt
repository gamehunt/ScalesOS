cmake_minimum_required(VERSION 3.23)

project(Scales-Libauth C)

add_library(auth SHARED 
	src/auth.c
)
set_property(TARGET auth PROPERTY POSITION_INDEPENDENT_CODE 1)

target_include_directories(auth PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/)
set_target_properties(auth PROPERTIES PUBLIC_HEADER "include/auth.h")

install(TARGETS auth
	PUBLIC_HEADER DESTINATION include/
)

install(TARGETS auth
	LIBRARY DESTINATION lib/
)
