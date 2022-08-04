cmake_minimum_required(VERSION 3.23)

project(ScalesBoot VERSION 1.0.0 LANGUAGES CXX C ASM)

find_library (GNU-EFI-LIB gnuefi REQUIRED) 
find_library (EFI-LIB efi REQUIRED) 

find_path (EFI-HEADERS efi REQUIRED)

find_file (GNU-EFI-LD  elf_x86_64_efi.lds HINTS /usr/lib REQUIRED)
find_file (GNU-EFI-CRT crt0-efi-x86_64.o  HINTS /usr/lib REQUIRED)

set(EFI-HEADERS ${EFI-HEADERS}/efi)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

include_directories(
    ${EFI-HEADERS} 
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
    ${CMAKE_CURRENT_BINARY_DIR} 
    ${CMAKE_INSTALL_PREFIX}/include
)

add_library(bootl SHARED)

set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_LINKER> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

set_target_properties(bootl PROPERTIES PREFIX "")
set_target_properties(bootl PROPERTIES PUBLIC_HEADER "include/boot.h")

target_sources(bootl PUBLIC 
    src/efi_wrappers.c 
    src/cfg_parser.c 
    src/main.c
)

target_compile_options(bootl PUBLIC -fpic -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args)

target_link_options(bootl PUBLIC -nostdlib ${GNU-EFI-CRT} -T${GNU-EFI-LD} -shared -Bsymbolic)
target_link_libraries(bootl PUBLIC ${GNU-EFI-LIB} ${EFI-LIB})

add_custom_command(TARGET bootl COMMAND objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 bootl.so bootl.efi VERBATIM)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/bootl.efi DESTINATION bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/boot.cfg  DESTINATION .)
install(TARGETS bootl PUBLIC_HEADER DESTINATION include/boot)