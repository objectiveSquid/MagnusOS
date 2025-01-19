#!/bin/bash

TARGET=$1

STAGE_2_LOCATION_OFFSET=480

# create empty disk file
dd if=/dev/zero of=$TARGET bs=512 count=2880

# calculate reserved sectors for stage 2
STAGE2_SIZE=$(stat -c%s ${BUILD_DIR}/bootloader_stage_2.bin)
STAGE2_SECTOR_COUNT=$(( ( $STAGE2_SIZE + 511 ) / 512 ))
RESERVED_SECTOR_COUNT=$(( 1 + $STAGE2_SECTOR_COUNT ))

# make fat12 fs
mkfs.fat -F 12 -R $RESERVED_SECTOR_COUNT -n "MagnusOS" $TARGET

# bootloader
dd if=${BUILD_DIR}/bootloader_stage_1.bin of=$TARGET conv=notrunc bs=1 count=3
dd if=${BUILD_DIR}/bootloader_stage_1.bin of=$TARGET conv=notrunc bs=1 seek=62 skip=62
dd if=${BUILD_DIR}/bootloader_stage_2.bin of=$TARGET conv=notrunc bs=512 seek=1

# write lba of stage 2 to bootloader
echo "01 00 00 00" | xxd -r -p | dd of=$TARGET conv=notrunc bs=1 seek=$STAGE_2_LOCATION_OFFSET
printf "%x" $STAGE2_SECTOR_COUNT | xxd -r -p | dd of=$TARGET conv=notrunc bs=1 seek=$(( $STAGE_2_LOCATION_OFFSET + 4 ))

# copy files
mcopy -i $TARGET ${BUILD_DIR}/kernel.bin "::kernel.bin"
mmd -i $TARGET "::info"
mcopy -i $TARGET README.md "::info/readme.md"