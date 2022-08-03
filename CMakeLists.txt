cmake_minimum_required(VERSION 3.23)

project(Scales)

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/env)

add_subdirectory(bootloader)
add_subdirectory(kernel)

install(DIRECTORY scripts/ DESTINATION . PATTERN "scripts/*" PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ) 
install(DIRECTORY shared/  DESTINATION include/shared PATTERN "shared/*")