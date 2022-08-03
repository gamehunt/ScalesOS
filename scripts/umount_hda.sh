#!/bin/bash

sudo -u root umount disk/boot
sudo -u root umount disk

sudo kpartx -d disk.img
