#!/bin/bash
set -e

/bin/bash mount_hda.sh

if [ ! -d "disk/boot/EFI/BOOT" ]; then
    sudo -u root mkdir -p disk/boot/EFI/BOOT
fi

sudo -u root cp bin/bootl.efi disk/boot/EFI/BOOT/BOOTX64.EFI

sync