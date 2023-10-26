cmake_minimum_required(VERSION 3.23)

project(Scales-Libcompose C)

add_library(compose OBJECT src/compose.c)
set_property(TARGET compose PROPERTY POSITION_INDEPENDENT_CODE 1)

target_include_directories(compose PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/)
set_target_properties(compose PROPERTIES PUBLIC_HEADER "include/compose.h")

add_library(composest STATIC $<TARGET_OBJECTS:compose>)
add_library(composeso SHARED $<TARGET_OBJECTS:compose>)

target_link_libraries(composeso PRIVATE fb)

set_target_properties(composest PROPERTIES OUTPUT_NAME compose)
set_target_properties(composeso PROPERTIES OUTPUT_NAME compose)

install(TARGETS compose
	PUBLIC_HEADER DESTINATION include/
)

install(TARGETS composest composeso
	LIBRARY DESTINATION lib/
)
