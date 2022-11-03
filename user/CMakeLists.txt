cmake_minimum_required(VERSION 3.23)
include(ExternalProject)

project(Scales-Userspace VERSION 1.0.0)

ExternalProject_Add(
    init
    SOURCE_DIR  ${PROJECT_SOURCE_DIR}/init/
    BINARY_DIR  ${CMAKE_CURRENT_BINARY_DIR}/init/
    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
)

install(CODE "MESSAGE(\"Userspace installed.\")")