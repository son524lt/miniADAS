# S2mini ESP32-S2 Build and Upload Makefile

# Configuration
CHIP := esp32s2
BAUD_RATE := 460800
FLASH_MODE := dio
FLASH_SIZE := 2MB
FLASH_FREQ := 80m

# Paths
BUILD_DIR := build
BOOTLOADER := $(BUILD_DIR)/bootloader/bootloader.bin
PARTITION_TABLE := $(BUILD_DIR)/partition_table/partition-table.bin
APP_BIN := $(BUILD_DIR)/S2mini.bin

# Default target
.PHONY: all
all: build upload

# Build target - compile the project using ESP-IDF
.PHONY: build
build:
	idf.py build

# Upload target - flash the device with compiled binaries
.PHONY: upload
upload: $(BOOTLOADER) $(PARTITION_TABLE) $(APP_BIN)
	python -m esptool \
		--chip $(CHIP) \
		-b $(BAUD_RATE) \
		--before default_reset \
		--after hard_reset \
		write_flash \
		--flash_mode $(FLASH_MODE) \
		--flash_size $(FLASH_SIZE) \
		--flash_freq $(FLASH_FREQ) \
		0x1000 $(BOOTLOADER) \
		0x8000 $(PARTITION_TABLE) \
		0x10000 $(APP_BIN)

# Clean build artifacts
.PHONY: clean
clean:
	idf.py fullclean

# Monitor serial output
.PHONY: monitor
monitor:
	idf.py monitor --no-reset

# Menuconfig - configure project settings
.PHONY: config
config:
	idf.py menuconfig

.PHONY: help
help:
	@echo "Available targets:"
	@echo "  make all            - Build and upload the project"
	@echo "  make build          - Build the project"
	@echo "  make upload         - Upload binaries to device (requires binaries to exist)"
	@echo "  make clean          - Clean build artifacts"
	@echo "  make monitor        - Monitor serial output"
	@echo "  make config         - Configure project settings"
