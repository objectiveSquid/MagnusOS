include build-scripts/config.mk

FILES_DIR=$(BUILD_SCRIPTS_DIR)/files
FILES_JSON=$(BUILD_SCRIPTS_DIR)/shared.json

.PHONY: copy_shared_files

copy_shared_files:
	python3 $(BUILD_SCRIPTS_DIR)/copy-shared-files.py $(FILES_JSON) $(FILES_DIR) src
