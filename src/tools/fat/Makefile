BUILD_DIR?=build
CC?=gcc

.PHONY: all tools_fat clean

all: tools_fat

tools_fat: $(BUILD_DIR)/tools/fat12
$(BUILD_DIR)/tools/fat12: always $(TOOLS_DIR)/fat/fat12.c
	mkdir -p $(BUILD_DIR)/tools
	$(CC) -g -o $(BUILD_DIR)/tools/fat12 $(TOOLS_DIR)/fat/fat12.c

clean:
	rm $(BUILD_DIR)/tools/fat12
