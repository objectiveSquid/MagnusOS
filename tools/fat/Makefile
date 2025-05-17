.PHONY: all tools_fat clean always

all: tools_fat

tools_fat: $(BUILD_DIR)/tools/fat12
$(BUILD_DIR)/tools/fat12: always
	$(CC) -g -o $(BUILD_DIR)/tools/fat12 $(TOOLS_DIR)/fat/fat12.c

always:
	echo "THIS SCRIPT IS DEPRECATED AND WILL BE REPLACED BY A SCONSCRIPT"
	mkdir -p $(BUILD_DIR)/tools

clean:
	rm -f $(BUILD_DIR)/tools/fat12
