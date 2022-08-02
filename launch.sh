#!/bin/bash

set -e

cd build
rm -rf *
cmake ..
make
cd ..

mcopy -D overwrite-all -i fat.img build/bootloader/bootl.efi ::/EFI/BOOT/BOOTX64.EFI
mkgpt -o hdimage.bin --image-size 4096 --part fat.img --type system
qemu-system-x86_64 -enable-kvm -m 1G -bios /usr/share/ovmf/x64/OVMF.fd -hda hdimage.bin -monitor stdio -display gtk -vga std
