echo "Creating ramdisk..."

tar -cvf scales.initrd ramdisk/
sudo -u root cp scales.initrd /mnt/boot