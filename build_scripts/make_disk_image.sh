#!/bin/bash

set -e

TARGET=$1
DISK_SIZE=$2

STAGE_2_LOCATION_OFFSET=480

DISK_SECTOR_COUNT=$(( ( ${DISK_SIZE} + 511 ) / 512 )) 

DISK_PARTITION_1_BEGIN=2048
DISK_PARTITION_1_END=$(( ${DISK_SECTOR_COUNT} - 1))

# create empty disk file
echo Creating $TARGET
dd if=/dev/zero of=$TARGET bs=512 count=$DISK_SECTOR_COUNT

# create partition table
echo Creating partition table on $TARGET
parted -s $TARGET mklabel msdos
echo Creating partition on $TARGET
parted -s $TARGET mkpart primary ${DISK_PARTITION_1_BEGIN}s ${DISK_PARTITION_1_END}s
echo Setting boot flag on $TARGET
parted -s $TARGET set 1 boot on

# calculate reserved sectors for stage 2
STAGE2_SIZE=$(stat -c%s ${BUILD_DIR}/bootloader_stage_2.bin)
STAGE2_SECTOR_COUNT=$(( ( ${STAGE2_SIZE} + 511 ) / 512 ))

if [ "$STAGE2_SECTOR_COUNT" -gt $(( $DISK_PARTITION_1_BEGIN - 1 )) ]; then
    echo "Stage 2 of bootloader is too large (${STAGE2_SECTOR_COUNT} sectors, only have ${DISK_PARTITION_1_BEGIN} sectors)"
    exit 2
fi

# stage 2 of bootloader
echo "Writing stage 2 of bootloader to $TARGET"
dd if=${BUILD_DIR}/bootloader_stage_2.bin of=$TARGET conv=notrunc bs=512 seek=1

# create loopback device
echo "Creating loopback device"
LOOPBACK_DEVICE=$(losetup -fP --show $TARGET)
TARGET_PARTITION="${LOOPBACK_DEVICE}p1"
echo "Created loopback device $LOOPBACK_DEVICE"

# make fat fs
echo Creating fat fs on $TARGET
mkfs.fat -F 12 -n "MagnusOS" $TARGET_PARTITION

# stage 1 of bootloader
echo "Writing stage 1 of bootloader to $TARGET"
dd if=${BUILD_DIR}/bootloader_stage_1.bin of=$TARGET_PARTITION conv=notrunc bs=1 count=3
dd if=${BUILD_DIR}/bootloader_stage_1.bin of=$TARGET_PARTITION conv=notrunc bs=1 seek=90 skip=90

# write lba of stage 2 to bootloader
echo "Writing lba of stage 2 to bootloader"
echo "01 00 00 00" | xxd -r -p | dd of=$TARGET_PARTITION conv=notrunc bs=1 seek=$STAGE_2_LOCATION_OFFSET
printf "%x" $STAGE2_SECTOR_COUNT | xxd -r -p | dd of=$TARGET_PARTITION conv=notrunc bs=1 seek=$(( $STAGE_2_LOCATION_OFFSET + 4 ))

# copy files
MOUNT_DIRECTORY=/tmp/magnusos
echo "Copying files to $TARGET (mounted on $MOUNT_DIRECTORY)"
mkdir -p $MOUNT_DIRECTORY
mount $TARGET_PARTITION $MOUNT_DIRECTORY

cp ${BUILD_DIR}/kernel.bin $MOUNT_DIRECTORY/kernel.bin
mkdir $MOUNT_DIRECTORY/info
cp README.md $MOUNT_DIRECTORY/info/readme.md
cp -r ${BUILD_DIR}/files/* $MOUNT_DIRECTORY/

umount $MOUNT_DIRECTORY
rm -rf $MOUNT_DIRECTORY

# destroy loopback device
echo Destroying loopback device
losetup -d $LOOPBACK_DEVICE
