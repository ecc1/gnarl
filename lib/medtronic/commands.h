#define LOG_LOCAL_LEVEL	ESP_LOG_DEBUG
#define TAG		"medtronic"
#include <esp_log.h>

typedef enum {
	CMD_ACK		  = 0x06,
	CMD_NAK		  = 0x15,
	CMD_WAKEUP	  = 0x5D,
	CMD_CLOCK	  = 0x70,
	CMD_BATTERY	  = 0x72,
	CMD_RESERVOIR	  = 0x73,
	CMD_CARB_UNITS	  = 0x88,
	CMD_GLUCOSE_UNITS = 0x89,
	CMD_CARB_RATIOS   = 0x8A,
	CMD_SENSITIVITIES = 0x8B,
	CMD_TARGETS_512   = 0x8C,
	CMD_MODEL	  = 0x8D,
	CMD_SETTINGS_512  = 0x91,
	CMD_BASAL_RATES   = 0x92,
	CMD_TEMP_BASAL	  = 0x98,
	CMD_TARGETS       = 0x9F,
	CMD_SETTINGS	  = 0xC0,
	CMD_STATUS	  = 0xCE,
} command_t;

void print_bytes(const char *msg, const uint8_t *buf, int count);
uint8_t *short_command(command_t cmd, int *len);
uint8_t *extended_response(command_t cmd, int *len);
