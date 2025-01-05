include build-scripts/config.mk

FONTS_DIR=$(BUILD_DIR)/files/fonts
HEADER_PATH=src/bootloader/stage_2/visual/rasterfont_sizes.h

.PHONY: generate_fonts

generate_fonts:
	python3 $(ROOT_DIR)/build-scripts/generate-fonts.py $(FONTS_DIR) $(HEADER_PATH)
