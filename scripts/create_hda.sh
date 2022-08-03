#!/bin/bash
set -e

if [ ! -f "disk.img" ]; then
    dd if=/dev/zero of=disk.img bs=1024k seek=2048 count=0
    sfdisk disk.img < disk.sfdisk
    sudo -u root kpartx -a disk.img
    sudo -u root mkdosfs -F 32 /dev/mapper/loop0p1
    sudo -u root mkfs.ext4     /dev/mapper/loop0p2
    sudo -u root kpartx -d disk.img
else
    echo "Disk already exits, skipping..."
fi