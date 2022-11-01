cmake_minimum_required(VERSION 3.23)
include(ExternalProject)

project(Scales-Modules VERSION 1.0.0)

ExternalProject_Add(
    test
    SOURCE_DIR  ${PROJECT_SOURCE_DIR}/test/
    BINARY_DIR  ${CMAKE_CURRENT_BINARY_DIR}/test/
    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
)

install(CODE "MESSAGE(\"Modules installed.\")")