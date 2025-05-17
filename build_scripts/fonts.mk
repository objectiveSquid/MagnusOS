include build_scripts/config.mk

FONTS_DIR=$(IMAGE_DIR)/generated_root/fonts
HEADER_PATH=src/kernel/visual/rasterfont_sizes.h

.PHONY: generate_fonts

generate_fonts:
	python3 $(BUILD_SCRIPTS_DIR)/generate-fonts.py $(FONTS_DIR) $(HEADER_PATH)
