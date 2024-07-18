TARGET_ASMFLAGS += -f elf
TARGET_CFLAGS += -ffreestanding -nostdlib -I.
TARGET_LIBS += -lgcc
TARGET_LINKFLAGS += -T linker.ld -nostdlib

HEADERS_C = ./arch/i686/isr_gen.h \
			$(shell find . -type f -name "*.h")
SOURCES_C = ./arch/i686/isr_gen.c \
			$(shell find . -type f -name "*.c")
OBJECTS_C = $(patsubst %.c, $(BUILD_DIR)/kernel/c/%.obj, $(SOURCES_C))

HEADERS_ASM = ./arch/i686/isr_gen.inc \
				$(shell find . -type f -name "*.inc")
SOURCES_ASM = $(shell find . -type f -name "*.asm")
OBJECTS_ASM = $(patsubst %.asm, $(BUILD_DIR)/kernel/asm/%.obj, $(SOURCES_ASM))

.PHONY: all kernel clean always

all: kernel

kernel: generate_isr $(BUILD_DIR)/kernel.bin
$(BUILD_DIR)/kernel.bin: $(OBJECTS_C) $(OBJECTS_ASM)
	$(TARGET_LD) $(TARGET_LINKFLAGS) -Wl,-Map=$(BUILD_DIR)/kernel.map -o $@ $^ $(TARGET_LIBS)

$(BUILD_DIR)/kernel/c/%.obj: %.c $(HEADERS_C) always
	$(TARGET_CC) $(TARGET_CFLAGS) -c -o $@ $<

$(BUILD_DIR)/kernel/asm/%.obj: %.asm $(HEADERS_ASM) always
	$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<

generate_isr:
	$(ROOT_DIR)/build-scripts/generate-isr.sh arch/i686

always:
	mkdir -p $(BUILD_DIR)/kernel/c
	mkdir -p $(BUILD_DIR)/kernel/c/hal
	mkdir -p $(BUILD_DIR)/kernel/c/arch/i686
	mkdir -p $(BUILD_DIR)/kernel/asm
	mkdir -p $(BUILD_DIR)/kernel/asm/arch/i686

clean:
	rm -f $(BUILD_DIR)/kernel.bin