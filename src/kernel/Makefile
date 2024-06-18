BUILD_DIR?=build
ASM?=nasm

.PHONY: all kernel clean

all: kernel

kernel: $(BUILD_DIR)/kernel.bin
$(BUILD_DIR)/kernel.bin:
	$(ASM) boot.asm -f bin -o $(BUILD_DIR)/kernel.bin

clean:
	rm $(BUILD_DIR)/kernel.bin