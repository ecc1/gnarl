#ifndef _COMMANDS_H
#define _COMMANDS_H

#include <stdint.h>
#include <string.h>
#include <time.h>

#define LOG_LOCAL_LEVEL	ESP_LOG_DEBUG
#define TAG		"medtronic"
#include <esp_log.h>

typedef enum {
	CMD_ACK			= 0x06,
	CMD_NAK			= 0x15,
	CMD_SET_ABS_TEMP_BASAL	= 0x4C,
	CMD_WAKEUP		= 0x5D,
	CMD_CLOCK		= 0x70,
	CMD_BATTERY		= 0x72,
	CMD_RESERVOIR		= 0x73,
	CMD_HISTORY		= 0x80,
	CMD_CARB_UNITS		= 0x88,
	CMD_GLUCOSE_UNITS	= 0x89,
	CMD_CARB_RATIOS		= 0x8A,
	CMD_SENSITIVITIES	= 0x8B,
	CMD_TARGETS_512		= 0x8C,
	CMD_MODEL		= 0x8D,
	CMD_SETTINGS_512	= 0x91,
	CMD_BASAL_RATES		= 0x92,
	CMD_TEMP_BASAL		= 0x98,
	CMD_TARGETS		= 0x9F,
	CMD_SETTINGS		= 0xC0,
	CMD_STATUS		= 0xCE,
} command_t;

uint8_t *short_command(command_t cmd, int *len);

uint8_t *long_command(command_t cmd, uint8_t *params, int params_len, int *len);

uint8_t *extended_response(command_t cmd, int *len);

uint8_t *download_page(command_t cmd, int page_num, int *len);

static inline int two_byte_be_int(uint8_t *p) {
	return (p[0] << 8) | p[1];
}

static inline int two_byte_le_int(uint8_t *p) {
	return (p[1] << 8) | p[0];
}

static inline insulin_t int_to_insulin(int n, int fam) {
	if (fam <= 22) {
		return 100 * n;
	}
	return 25 * n;
}

// Convert a number of half-hours into seconds.
static inline time_t half_hours(int n) {
	return 60 * 30 * n;
}

void print_bytes(const char *msg, const uint8_t *data, int len);

#endif // _COMMANDS_H
