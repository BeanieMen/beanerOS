#!/bin/bash

# Create a 512MB FAT32 disk image
dd if=/dev/zero of=disk.img bs=1M count=512 2>/dev/null

mkfs.vfat -F 32 disk.img

if [ -d initrd_files ]; then
    for file in initrd_files/*; do
        if [ -f "$file" ]; then
            mcopy -i disk.img "$file" ::
        fi
    done
fi

echo "Created 512MB FAT32 disk image with files from initrd_files/"
