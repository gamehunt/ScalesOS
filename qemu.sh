#!/bin/bash
set -e

[ ! -d sysroot ] || sudo -u root rm -rf sysroot
[ ! -d build ]   || sudo -u root rm -rf build

cd scripts
/bin/bash mount_hda.sh
cd ..

/bin/bash scripts/create_sysroot.sh

cmake -DCMAKE_TOOLCHAIN_FILE=toolchain/i686_elf_toolchain.cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -Bbuild
cd build
VERBOSE=1 make 
sudo -u root make install
/bin/bash ../scripts/create_ramdisk.sh
sync

cd ..
sudo -u root cp -a sysroot/. /mnt
sync

cd scripts
/bin/bash launch.sh