#!/bin/bash
set -e

/bin/bash create_hda.sh
/bin/bash umount_hda.sh

sudo -u root kpartx -a disk.img

if [ ! -d "disk" ]; then
    sudo -u root mkdir -p disk/boot
fi

sudo -u root mount /dev/mapper/loop0p2 disk
sudo -u root mount /dev/mapper/loop0p1 disk/boot