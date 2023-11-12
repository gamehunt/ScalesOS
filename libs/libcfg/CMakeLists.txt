cmake_minimum_required(VERSION 3.23)

project(Scales-Libcfg C)

add_library(cfg SHARED 
	src/cfg.c
)
set_property(TARGET cfg PROPERTY POSITION_INDEPENDENT_CODE 1)

target_include_directories(cfg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/)
set_target_properties(cfg PROPERTIES PUBLIC_HEADER "include/cfg.h")
target_link_libraries(cfg PRIVATE types)

install(TARGETS cfg
	PUBLIC_HEADER DESTINATION include/
)

install(TARGETS cfg
	LIBRARY DESTINATION lib/
)
