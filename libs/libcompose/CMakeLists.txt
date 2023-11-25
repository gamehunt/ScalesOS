cmake_minimum_required(VERSION 3.23)

project(Scales-Libcompose C)

add_library(compose SHARED 
	src/compose.c
	src/events.c
	src/request.c
)
set_property(TARGET compose PROPERTY POSITION_INDEPENDENT_CODE 1)
target_include_directories(compose PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/)
target_compile_definitions(compose PRIVATE __COMPOSE)
set_target_properties(compose PROPERTIES PUBLIC_HEADER "include/compose.h;include/events.h;include/request.h")

target_link_libraries(compose PRIVATE fb input types)

install(TARGETS compose
	PUBLIC_HEADER DESTINATION include/compose/
)

install(TARGETS compose
	LIBRARY DESTINATION lib/
)
