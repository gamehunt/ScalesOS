#!/bin/bash

sudo -u root umount /mnt > /dev/null 2>&1
sudo kpartx -d disk.img
