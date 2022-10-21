#!/bin/bash

set -e

sudo -u root cp grub.cfg /mnt/boot/grub/
sync

qemu-system-i386 -enable-kvm -m 1G -drive format=raw,file=disk.img -monitor stdio -display gtk -vga std -d cpu_reset -d cpu_reset
