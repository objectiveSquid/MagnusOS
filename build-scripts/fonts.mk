include build-scripts/config.mk

FONTS_DIR=$(BUILD_DIR)/files/fonts
HEADER_PATH=src/kernel/visual/rasterfont_sizes.h

.PHONY: generate_fonts

generate_fonts:
	python3 $(BUILD_SCRIPTS_DIR)/generate-fonts.py $(FONTS_DIR) $(HEADER_PATH)
