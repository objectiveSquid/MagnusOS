ASM=nasm
SRC_DIR=src
BUILD_DIR=build

.PHONY: image run run_gui debug
image: $(BUILD_DIR)/protected_mode_floppy.img

$(BUILD_DIR)/protected_mode_floppy.img: always $(BUILD_DIR)/protected_mode_bootloader.bin
	cp $(BUILD_DIR)/protected_mode_bootloader.bin $(BUILD_DIR)/protected_mode_floppy.img
	truncate -s 1440k $(BUILD_DIR)/protected_mode_floppy.img

$(BUILD_DIR)/protected_mode_bootloader.bin: always src/main.asm
	$(ASM) src/main.asm -f bin -o $(BUILD_DIR)/protected_mode_bootloader.bin

run_gui: image
	bash -c "unset GTK_PATH; unset GIO_MODULE_DIR; qemu-system-i386 -fda $(BUILD_DIR)/protected_mode_floppy.img"

run: image
	qemu-system-i386 -fda $(BUILD_DIR)/protected_mode_floppy.img -nographic

always:
	mkdir -p $(BUILD_DIR)

debug: image
	bash -c "unset GTK_PATH; unset GIO_MODULE_DIR; bochs -f bochs_config"
