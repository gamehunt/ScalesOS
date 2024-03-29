cmake_minimum_required(VERSION 3.23)
include(ExternalProject)

project(Scales-Modules VERSION 1.0.0)

ExternalProject_Add(
    test
    SOURCE_DIR  ${PROJECT_SOURCE_DIR}/test/
    BINARY_DIR  ${CMAKE_CURRENT_BINARY_DIR}/test/
    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
)

ExternalProject_Add(
    ata
    SOURCE_DIR  ${PROJECT_SOURCE_DIR}/ata/
    BINARY_DIR  ${CMAKE_CURRENT_BINARY_DIR}/ata/
    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
)

ExternalProject_Add(
    dospart
    SOURCE_DIR  ${PROJECT_SOURCE_DIR}/dospart/
    BINARY_DIR  ${CMAKE_CURRENT_BINARY_DIR}/dospart/
    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
)

ExternalProject_Add(
    gptpart
    SOURCE_DIR  ${PROJECT_SOURCE_DIR}/gptpart/
    BINARY_DIR  ${CMAKE_CURRENT_BINARY_DIR}/gptpart/
    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
)

ExternalProject_Add(
    ext2
    SOURCE_DIR  ${PROJECT_SOURCE_DIR}/ext2/
    BINARY_DIR  ${CMAKE_CURRENT_BINARY_DIR}/ext2/
    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
)

ExternalProject_Add(
    qemuvideo
    SOURCE_DIR  ${PROJECT_SOURCE_DIR}/qemuvideo/
    BINARY_DIR  ${CMAKE_CURRENT_BINARY_DIR}/qemuvideo/
    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
)

install(CODE "MESSAGE(\"Modules installed.\")")
