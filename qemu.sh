#!/bin/bash
set -e

[ ! -d build ] || rm -rf build

cd scripts
/bin/bash mount_hda.sh
cd ..

cmake -DCMAKE_TOOLCHAIN_FILE=toolchain/i686_elf_toolchain.cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -Bbuild
cd build
VERBOSE=1 make 
sudo -u root make install
sync

cd ../scripts
/bin/bash launch.sh