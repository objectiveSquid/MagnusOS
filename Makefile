include build-scripts/config.mk

.PHONY: all disk_image kernel bootloader clean always tools

all: disk_image

include build-scripts/toolchain.mk
include build-scripts/fonts.mk

export PATH := $(PATH):$(TOOLCHAIN_PREFIX)/bin

#
# Disk image
#
disk_image: $(BUILD_DIR)/main_disk.raw
$(BUILD_DIR)/main_disk.raw: bootloader kernel generate_fonts
	./build-scripts/make_disk_image.sh $@ $(MAKE_DISK_SIZE)

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
$(BUILD_DIR)/bootloader_stage_2.bin: always generate_fonts
	$(MAKE) -C src/bootloader/stage_2 BUILD_DIR=$(abspath $(BUILD_DIR))

#
# Kernel
#
kernel: $(BUILD_DIR)/kernel.bin
$(BUILD_DIR)/kernel.bin: always
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