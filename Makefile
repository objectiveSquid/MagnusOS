include build-scripts/config.mk

.PHONY: all floppy_image kernel bootloader generate_all_files clean always tools

all: floppy_image

include build-scripts/toolchain.mk
include build-scripts/fonts.mk
include build-scripts/shared.mk

export PATH := $(PATH):$(TOOLCHAIN_PREFIX)/bin

#
# Generate all files (unused, just for convenience)
#
generate_all_files: generate_fonts copy_shared_files

#
# Floppy image
#
floppy_image: $(BUILD_DIR)/main_floppy.img
$(BUILD_DIR)/main_floppy.img: bootloader kernel
	dd if=/dev/zero of=$(BUILD_DIR)/main_floppy.img bs=512 count=2880
	mkfs.fat -F 12 -n "MagnusOS" $(BUILD_DIR)/main_floppy.img
	dd if=$(BUILD_DIR)/bootloader_stage_1.bin of=$(BUILD_DIR)/main_floppy.img conv=notrunc
	mcopy -i $(BUILD_DIR)/main_floppy.img $(BUILD_DIR)/bootloader_stage_2.bin "::stage2.bin"
	mcopy -i $(BUILD_DIR)/main_floppy.img $(BUILD_DIR)/kernel.bin "::kernel.bin"
	mkdir -p "$(BUILD_DIR)/files/info"
	cp README.md "$(BUILD_DIR)/files/info/readme.md"
	mcopy -s -i $(BUILD_DIR)/main_floppy.img $(BUILD_DIR)/files/* "::"

#
# Bootloader
#
bootloader: bootloader_stage_1 bootloader_stage_2

# Stage 1
bootloader_stage_1: $(BUILD_DIR)/bootloader_stage_1.bin
$(BUILD_DIR)/bootloader_stage_1.bin: always
	$(MAKE) -C src/bootloader/stage_1 BUILD_DIR=$(abspath $(BUILD_DIR))

# Stage 2
bootloader_stage_2: $(BUILD_DIR)/bootloader_stage_2.bin
$(BUILD_DIR)/bootloader_stage_2.bin: always copy_shared_files
	$(MAKE) -C src/bootloader/stage_2 BUILD_DIR=$(abspath $(BUILD_DIR))

#
# Kernel
#
kernel: $(BUILD_DIR)/kernel.bin
$(BUILD_DIR)/kernel.bin: always generate_fonts copy_shared_files
	$(MAKE) -C src/kernel BUILD_DIR=$(abspath $(BUILD_DIR))

#
# FAT Tools
#
tools: tools_fat

tools_fat: $(BUILD_DIR)/tools/fat
$(BUILD_DIR)/tools/fat: always
	$(MAKE) -C tools/fat BUILD_DIR=$(abspath $(BUILD_DIR))

#
# Always
#
always:
	mkdir -p $(BUILD_DIR)

#
# Clean
#
clean:
	$(MAKE) -C src/bootloader/stage_1 BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	$(MAKE) -C src/bootloader/stage_2 BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	$(MAKE) -C src/kernel BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	$(MAKE) -C tools/fat BUILD_DIR=$(abspath $(BUILD_DIR)) clean
	$(MAKE) -C build-scripts clean
	rm -rf $(BUILD_DIR)