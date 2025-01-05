TARGET_ASMFLAGS += -f elf
TARGET_CFLAGS += -ffreestanding -nostdlib -I.
TARGET_LIBS += -lgcc
TARGET_LINKFLAGS += -T linker.ld -nostdlib

HEADERS_C = $(shell find . -type f -name "*.h")
SOURCES_C = $(shell find . -type f -name "*.c")
OBJECTS_C = $(patsubst %.c, $(BUILD_DIR)/bootloader_stage_2/c/%.obj, $(SOURCES_C))

HEADERS_ASM = $(shell find . -type f -name "*.inc")
SOURCES_ASM = $(shell find . -type f -name "*.asm")
OBJECTS_ASM = $(patsubst %.asm, $(BUILD_DIR)/bootloader_stage_2/asm/%.obj, $(SOURCES_ASM))

.PHONY: all bootloader_stage_2 clean always

all: bootloader_stage_2

bootloader_stage_2: $(BUILD_DIR)/bootloader_stage_2.bin
$(BUILD_DIR)/bootloader_stage_2.bin: $(OBJECTS_C) $(OBJECTS_ASM)
	$(TARGET_LD) $(TARGET_LINKFLAGS) -Wl,-Map=$(BUILD_DIR)/bootloader_stage_2.map -o $@ $^ $(TARGET_LIBS)

$(BUILD_DIR)/bootloader_stage_2/c/%.obj: %.c $(HEADERS_C) always
	$(TARGET_CC) $(TARGET_CFLAGS) -c -o $@ $<

$(BUILD_DIR)/bootloader_stage_2/asm/%.obj: %.asm $(HEADERS_ASM) always
	$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<

always:
	mkdir -p $(BUILD_DIR)/bootloader_stage_2/c
	mkdir -p $(BUILD_DIR)/bootloader_stage_2/c/visual
	mkdir -p $(BUILD_DIR)/bootloader_stage_2/c/visual/rasterfonts
	mkdir -p $(BUILD_DIR)/bootloader_stage_2/asm

clean:
	rm -f $(BUILD_DIR)/bootloader_stage_2.bin