#!/bin/bash

set -e

sudo -u root cp grub.cfg /mnt/boot/grub/
sync

qemu-system-i386 -m 1G -smp 2 -s -drive format=raw,file=disk.img \
				 -monitor stdio -display gtk -vga std \
				 -d cpu_reset -audiodev sdl,id=audio0 -machine pcspk-audiodev=audio0 \
				 -enable-kvm \
				 # -S
