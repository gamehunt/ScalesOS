cmake_minimum_required(VERSION 3.23)
include(ExternalProject)

project(Scales-Userspace VERSION 1.0.0)

ExternalProject_Add(
    init
    SOURCE_DIR  ${PROJECT_SOURCE_DIR}/init/
    BINARY_DIR  ${CMAKE_CURRENT_BINARY_DIR}/init/
    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
)

ExternalProject_Add(
    scsh
    SOURCE_DIR  ${PROJECT_SOURCE_DIR}/scsh/
    BINARY_DIR  ${CMAKE_CURRENT_BINARY_DIR}/scsh/
    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
)

ExternalProject_Add(
    splash
    SOURCE_DIR  ${PROJECT_SOURCE_DIR}/splash/
    BINARY_DIR  ${CMAKE_CURRENT_BINARY_DIR}/splash/
    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
)

ExternalProject_Add(
    terminal
    SOURCE_DIR  ${PROJECT_SOURCE_DIR}/terminal/
    BINARY_DIR  ${CMAKE_CURRENT_BINARY_DIR}/terminal/
    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
)


add_executable(getty    getty.c)
add_executable(stty     stty.c)
add_executable(insmod   insmod.c)
add_executable(login    login.c)
add_executable(motd     motd.c)
add_executable(mount    mount.c)
add_executable(umount   umount.c)
add_executable(scfetch  scfetch.c)
add_executable(reboot   reboot.c)
add_executable(shutdown shutdown.c)
add_executable(ls ls.c)
add_executable(cat cat.c)
add_executable(echo echo.c)
add_executable(pwd pwd.c)

install(TARGETS getty stty mount umount insmod motd login scfetch reboot shutdown ls cat echo pwd DESTINATION bin/)
install(TARGETS mount DESTINATION ramdisk/bin/)

install(CODE "MESSAGE(\"Userspace installed.\")")
