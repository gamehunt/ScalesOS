cmake_minimum_required(VERSION 3.23)

project(Scales-Libfb C)

add_library(fb OBJECT src/fb.c)
set_property(TARGET fb PROPERTY POSITION_INDEPENDENT_CODE 1)

add_library(fbst STATIC $<TARGET_OBJECTS:fb>)
add_library(fbso SHARED $<TARGET_OBJECTS:fb>)

set_target_properties(fbst PROPERTIES OUTPUT_NAME fb)
set_target_properties(fbso PROPERTIES OUTPUT_NAME fb)

install(TARGETS fbst fbso DESTINATION lib/)
