set(CMAKE_SYSTEM_NAME Scales)

set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

set(CMAKE_FIND_ROOT_PATH  /hdd/Projects/Scales/sysroot)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_ASM_NASM_COMPILER /usr/bin/nasm) 
set(CMAKE_ASM_NASM_SOURCE_FILE_EXTENSIONS asm) 
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf)

set(CMAKE_C_COMPILER /usr/local/cross/bin/i686-scales-gcc)
set(CMAKE_C_COMPILER_TARGET i686-scales)

set(CMAKE_LINKER /usr/local/cross/bin/i686-scales-gcc)