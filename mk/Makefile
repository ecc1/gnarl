PROJECT_NAME := xyzzy

BOARD_VARIANT := ttgo-lora32-v1

# Make sure board-specific pin definitions are used instead of the ones in variants/esp32
CPPFLAGS += -I $(abspath components/arduino-esp32/variants/${BOARD_VARIANT})

EXTRA_COMPONENT_DIRS = lib

include $(IDF_PATH)/make/project.mk
