#!/bin/bash
set -e

if [ ! -f "disk.img" ]; then
    dd if=/dev/zero of=disk.img bs=1024k seek=2048 count=0
    sfdisk disk.img < disk.sfdisk

    sudo -u root kpartx -a disk.img
    
    yes | sudo -u root mke2fs /dev/mapper/loop0p1
    sync
    
    LOOPDEV=$(sudo -u root losetup -f)

    sudo -u root losetup $LOOPDEV disk.img 
    sudo -u root mount /dev/mapper/loop0p1 /mnt

    sudo -u root grub-install --target=i386-pc --root-directory=/mnt --no-floppy --modules="normal part_msdos ext2 multiboot" $LOOPDEV
    sudo -u root cp grub.cfg /mnt/boot/grub/
    sync

    sudo -u root losetup -d $LOOPDEV
    sudo -u root umount /mnt

    sudo -u root kpartx -d disk.img
    
else
    echo "Disk already exits, skipping..."
fi