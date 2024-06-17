ASM=nasm
CC="gcc"

SRC_DIR=src
BUILD_DIR=build
TOOLS_DIR=$(SRC_DIR)/tools

.PHONY: all floppy_image kernel bootloader clean always tools_fat

all: floppy_image tools_fat

# Floppy image
floppy_image: $(BUILD_DIR)/main_floppy.img
$(BUILD_DIR)/main_floppy.img: bootloader kernel
	dd if=/dev/zero of=$(BUILD_DIR)/main_floppy.img bs=512 count=2880
	mkfs.fat -F 12 -n "MagnusOS" $(BUILD_DIR)/main_floppy.img
	dd if=$(BUILD_DIR)/bootloader.bin of=$(BUILD_DIR)/main_floppy.img conv=notrunc
	mcopy -i $(BUILD_DIR)/main_floppy.img $(BUILD_DIR)/kernel.bin "::kernel.bin"
	mcopy -i $(BUILD_DIR)/main_floppy.img test.txt "::test.txt"

# Bootloader
bootloader: $(BUILD_DIR)/bootloader.bin
$(BUILD_DIR)/bootloader.bin: always
	$(ASM) $(SRC_DIR)/bootloader/boot.asm -f bin -o $(BUILD_DIR)/bootloader.bin

# Kernel
kernel: $(BUILD_DIR)/kernel.bin
$(BUILD_DIR)/kernel.bin: always
	$(ASM) $(SRC_DIR)/kernel/main.asm -f bin -o $(BUILD_DIR)/kernel.bin

# FAT Tools
tools_fat: $(BUILD_DIR)/tools/fat12
$(BUILD_DIR)/tools/fat12: always $(TOOLS_DIR)/fat/fat12.c
	mkdir -p $(BUILD_DIR)/tools
	$(CC) -g -o $(BUILD_DIR)/tools/fat12 $(TOOLS_DIR)/fat/fat12.c

# Always
always:
	mkdir -p $(BUILD_DIR)

# Clean
clean:
	rm -r $(BUILD_DIR)/