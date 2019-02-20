PROJECT_NAME := xyzzy

# Make sure board-specific pin definitions are used instead of the ones in variants/esp32
CPPFLAGS += -I $(abspath components/arduino-esp32/variants/ttgo-lora32-v1)

include $(IDF_PATH)/make/project.mk
