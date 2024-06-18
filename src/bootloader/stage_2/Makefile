BUILD_DIR?=build
ASM?=nasm

.PHONY: all bootloader_stage_2 clean

all: bootloader_stage_2

bootloader_stage_2: $(BUILD_DIR)/bootloader_stage_2.bin
$(BUILD_DIR)/bootloader_stage_2.bin:
	$(ASM) main.asm -f bin -o $(BUILD_DIR)/bootloader_stage_2.bin

clean:
	rm $(BUILD_DIR)/bootloader_stage_2.bin