#!/bin/bash

set -e

IMAGE_NAME="emu/sdcard.image"
IMAGE_SIZE_MB=32
MOUNT_POINT="emu/mnt"
FILE_TO_COPY="$1"

if [ -z "$FILE_TO_COPY" ]; then
  echo "Usage: $0 <file-to-copy>"
  exit 1
fi

dd if=/dev/zero of="$IMAGE_NAME" bs=1M count="$IMAGE_SIZE_MB"
mkfs.vfat -F 16 "$IMAGE_NAME"
mkdir -p "$MOUNT_POINT"
sudo mount -o loop "$IMAGE_NAME" "$MOUNT_POINT"
sudo mkdir -p "$MOUNT_POINT/PALM/Launcher"
sudo cp "$FILE_TO_COPY" "$MOUNT_POINT/PALM/Launcher/"
sync
sudo umount "$MOUNT_POINT"
rm -Rf "$MOUNT_POINT"

echo "Done! File '$FILE_TO_COPY' copied into '$IMAGE_NAME'."
