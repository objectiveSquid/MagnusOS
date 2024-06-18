BUILD_DIR?=build
ASM?=nasm

.PHONY: all bootloader_stage_2 clean

all: bootloader_stage_1

bootloader_stage_1: $(BUILD_DIR)/bootloader_stage_1.bin
$(BUILD_DIR)/bootloader_stage_1.bin:
	$(ASM) boot.asm -f bin -o $(BUILD_DIR)/bootloader_stage_1.bin

clean:
	rm $(BUILD_DIR)/bootloader_stage_1.bin