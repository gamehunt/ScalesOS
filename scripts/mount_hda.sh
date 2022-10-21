#!/bin/bash
set -e

/bin/bash create_hda.sh
/bin/bash umount_hda.sh

sudo -u root kpartx -a disk.img
sudo -u root mount /dev/mapper/loop0p1 /mnt
