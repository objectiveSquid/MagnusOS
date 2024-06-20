BUILD_DIR?=build
ASM?=nasm
ASMFLAGS?=-f obj
CC16?=/usr/bin/watcom/binl64/wcc
CFLAGS16?=-4 -d3 -s -wx -ms -zl -zq
LD16?=/usr/bin/watcom/binl64/wlink

SOURCES_C=$(wildcard *.c)
SOURCES_ASM=$(wildcard *.asm)
OBJECTS_C=$(patsubst %.c, $(BUILD_DIR)/bootloader_stage_2/c/%.obj, $(SOURCES_C))
OBJECTS_ASM=$(patsubst %.asm, $(BUILD_DIR)/bootloader_stage_2/asm/%.obj, $(SOURCES_ASM))

.PHONY: all bootloader_stage_2 clean always

all: bootloader_stage_2

bootloader_stage_2: $(BUILD_DIR)/bootloader_stage_2.bin

$(BUILD_DIR)/bootloader_stage_2.bin: $(OBJECTS_ASM) $(OBJECTS_C)
	$(LD16) NAME $(BUILD_DIR)/bootloader_stage_2.bin FILE \{ $(OBJECTS_ASM) $(OBJECTS_C) \} OPTION MAP=$(BUILD_DIR)/bootloader_stage_2.map @linker.lnk

$(BUILD_DIR)/bootloader_stage_2/c/%.obj: %.c always
	$(CC16) $(CFLAGS16) -fo=$@ $<

$(BUILD_DIR)/bootloader_stage_2/asm/%.obj: %.asm always
	$(ASM) $(ASMFLAGS) -o $@ $<

always:
	mkdir -p $(BUILD_DIR)/bootloader_stage_2/c
	mkdir -p $(BUILD_DIR)/bootloader_stage_2/asm

clean:
	rm $(BUILD_DIR)/bootloader_stage_2.bin