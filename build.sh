#!/bin/bash
set -e

# [ ! -d sysroot ] || sudo -u root rm -rf sysroot
[ ! -d build ]   || sudo -u root rm -rf build

cd scripts
/bin/bash mount_hda.sh
cd ..


rm -rf sysroot/etc || 1
rm -rf sysroot/ramdisk/etc || 1

cmake -Bbuild -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON
cmake --build build
cmake --build build --target resources
cmake --build build --target ramdisk
cmake --build build --target compiler_hints
sudo -u root cmake --build build --target install

sync
