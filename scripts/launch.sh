#!/bin/bash

set -e

if [ ! -f "/usr/share/ovmf/x64/OVMF.fd" ]; then
    echo "OVMF required to run this script."
    exit 1
fi

/bin/bash install.sh

qemu-system-x86_64 -enable-kvm -m 1G -bios /usr/share/ovmf/x64/OVMF.fd -drive format=raw,file=disk.img -monitor stdio -display gtk -vga std 
