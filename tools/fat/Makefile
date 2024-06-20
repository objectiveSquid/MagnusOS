BUILD_DIR?=build
TOOLS_DIR?=../../tools
CC?=gcc

.PHONY: all tools_fat clean always

all: tools_fat

tools_fat: $(BUILD_DIR)/tools/fat12
$(BUILD_DIR)/tools/fat12: always
	pwd
	$(CC) -g -o $(BUILD_DIR)/tools/fat12 ./$(TOOLS_DIR)/fat/fat12.c

always:
	mkdir -p $(BUILD_DIR)/tools

clean:
	rm $(BUILD_DIR)/tools/fat12
