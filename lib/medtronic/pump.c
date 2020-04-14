#include <stdint.h>
#include <stdio.h>

#include "medtronic.h"
#include "commands.h"

static inline glucose_t int_to_glucose(int n, glucose_units_t units) {
	switch (units) {
	case MG_PER_DL:
		return n;
	case MMOL_PER_L:
		// Convert 10x mmol/L to μmol/L
		return 100 * n;
	default:
		ESP_LOGE(TAG, "unknown glucose unit %d", units);
		abort();
	}
}

static int cached_pump_family;

int pump_family(void) {
	if (cached_pump_family == 0) {
		pump_model();
	}
	return cached_pump_family;
}

int pump_basal_rates(basal_rate_t *r, int max) {
	int n;
	uint8_t *data = extended_response(CMD_BASAL_RATES, &n);
	int count = 0;
	for (int i = 0; i < n - 2 && count < max; i += 3, count++, r++) {
		int rate = int_to_insulin(two_byte_le_int(&data[i]), 23);
		int t = data[i + 2];
		// Don't stop if the 00:00 rate happens to be zero.
		if (i > 1 && rate == 0 && t == 0) {
			break;
		}
		r->start = half_hours(t);
		r->rate = rate;
	}
	return count;
}

int pump_battery(void) {
	int n;
	uint8_t *data = short_command(CMD_BATTERY, &n);
	if (!data || n < 4 || data[0] != 3) {
		return -1;
	}
	return two_byte_be_int(&data[2]) * 10;
}

int pump_carb_ratios(carb_ratio_t *r, int max) {
	// Call this before the main command so the response buffer isn't overwritten.
	int fam = pump_family();
	int n;
	uint8_t *data = short_command(CMD_CARB_RATIOS, &n);
	if (!data || n < 2) {
		ESP_LOGE(TAG, "pump_carb_ratios: data %p length %d", data, n);
		return 0;
	}
	int step = fam <= 22 ? 2 : 3;
	int num = data[0] - 1;
	if (step + num >= n) {
		ESP_LOGE(TAG, "pump_carb_ratios: invalid length field (%d) for %d-byte packet", num, n);
		return 0;
	}
	if (num % step != 0) {
		ESP_LOGE(TAG, "pump_carb_ratios: length field (%d) not divisible by %d", num, step);
		return 0;
	}
	carb_units_t units = data[1];
	data += step;
	int count = 0;
	for (int i = 0; i <= num - step && count < max; i += step, count++, r++) {
		int t = data[i];
		if (t == 0 && count != 0) {
			break;
		}
		r->start = half_hours(t);
		r->units = units;
		if (fam <= 22) {
			int v = data[i + 1];
			switch (units) {
			case GRAMS:
				r->ratio = 10 * v;
				break;
			case EXCHANGES:
				r->ratio = 100 * v;
				break;
			default:
				ESP_LOGE(TAG, "pump_carb_ratios: unknown carb unit %d", units);
				return 0;
			}
		} else {
			r->ratio = two_byte_be_int(&data[i + 1]);
		}
	}
	return count;
}

carb_units_t pump_carb_units(void) {
	int n;
	uint8_t *data = short_command(CMD_CARB_UNITS, &n);
	if (!data || n < 2 || data[0] != 1) {
		return -1;
	}
	return data[1];
}

time_t pump_clock(void) {
	int n;
	uint8_t *data = short_command(CMD_CLOCK, &n);
	if (!data || n < 8 || data[0] != 7) {
		return -1;
	}
	struct tm tm = {
		.tm_hour = data[1],
		.tm_min = data[2],
		.tm_sec = data[3],
		.tm_year = two_byte_be_int(&data[4]) - 1900,
		.tm_mon = data[6] - 1,
		.tm_mday = data[7],
		.tm_isdst = -1,
	};
	return mktime(&tm);
}

glucose_units_t pump_glucose_units(void) {
	int n;
	uint8_t *data = short_command(CMD_GLUCOSE_UNITS, &n);
	if (!data || n < 2 || data[0] != 1) {
		return -1;
	}
	return data[1];
}

uint8_t *pump_history_page(int page_num) {
	int n;
	uint8_t *data = download_page(CMD_HISTORY, page_num, &n);
	if (data == 0 || n != HISTORY_PAGE_SIZE) {
		ESP_LOGE(TAG, "pump_history_page: data %p length %d", data, n);
		return 0;
	}
	return data;
}

int pump_model(void) {
	int n;
	uint8_t *data = short_command(CMD_MODEL, &n);
	if (!data || n < 2) {
		return -1;
	}
	int k = data[1];
	if (n < 2 + k) {
		return -1;
	}
	int model = 0;
	for (int i = 2; i < 2 + k; i++) {
		model = 10*model + data[i] - '0';
	}
	cached_pump_family = model % 100;
	return model;
}

int pump_reservoir(void) {
	// Call this before the main command so the response buffer isn't overwritten.
	int fam = pump_family();
	int n;
	uint8_t *data = short_command(CMD_RESERVOIR, &n);
	if (!data) {
		return -1;
	}
	if (fam <= 22) {
		if (n < 3 || data[0] != 2) {
			return -1;
		}
		return two_byte_be_int(&data[1]) * 100;
	}
	if (n < 5 || data[0] != 4) {
		return -1;
	}
	return two_byte_be_int(&data[3]) * 25;
}

int pump_sensitivities(sensitivity_t *r, int max) {
	int n;
	uint8_t *data = short_command(CMD_SENSITIVITIES, &n);
	if (!data || n < 2) {
		ESP_LOGE(TAG, "pump_sensitivities: data %p length %d", data, n);
		return 0;
	}
	int num = data[0] - 1;
	if (2 + num >= n) {
		ESP_LOGE(TAG, "pump_sensitivities: invalid length field (%d) for %d-byte packet", num, n);
		return 0;
	}
	if (num % 2 != 0) {
		ESP_LOGE(TAG, "pump_sensitivities: length field (%d) not divisible by 2", num);
		return 0;
	}
	glucose_units_t units = data[1];
	data += 2;
	int count = 0;
	for (int i = 0; i < num - 1 && count < max; i += 2, count++, r++) {
		int v = data[i];
		int t = v & 0x3F;
		if (t == 0 && count != 0) {
			break;
		}
		int s = (((v >> 6) & 0x1) << 8) | data[i + 1];
		r->start = half_hours(t);
		r->units = units;
		r->sensitivity = int_to_glucose(s, units);
	}
	return count;
}

int pump_settings(settings_t *r) {
	// Call this before the main command so the response buffer isn't overwritten.
	int fam = pump_family();
	command_t cmd = fam <= 12 ? CMD_SETTINGS_512 : CMD_SETTINGS;
	int n;
	uint8_t *data = short_command(cmd, &n);
	if (fam <= 12) {
		if (!data || n < 19 || data[0] != 18) {
			return -1;
		}
		r->max_bolus = int_to_insulin(data[6], 22);
		r->max_basal = int_to_insulin(two_byte_be_int(&data[7]), 23);
		// Response indicates only regular or fast-acting.
		r->dia = data[18] ? 8 : 6;
	} else if (fam <= 22) {
		if (!data || n < 22 || data[0] != 21) {
			return -1;
		}
		r->max_bolus = int_to_insulin(data[6], 22);
		r->max_basal = int_to_insulin(two_byte_be_int(&data[7]), 23);
		r->dia = data[18];
	} else {
		if (!data || n < 26 || data[0] != 25) {
			return -1;
		}
		r->max_bolus = int_to_insulin(data[7], 22);
		r->max_basal = int_to_insulin(two_byte_be_int(&data[8]), 23);
		r->dia = data[18];
	}
	r->temp_basal_type = data[14];
	return 0;
}

int pump_status(status_t *r) {
	int n;
	uint8_t *data = short_command(CMD_STATUS, &n);
	if (!data || n < 4 || data[0] != 3) {
		return -1;
	}
	r->code = data[1];
	r->bolusing = data[2] == 1;
	r->suspended = data[3] == 1;
	return 0;
}

int pump_targets(target_t *r, int max) {
	// Call this before the main command so the response buffer isn't overwritten.
	int fam = pump_family();
	command_t cmd = fam <= 12 ? CMD_TARGETS_512 : CMD_TARGETS;
	int n;
	uint8_t *data = short_command(cmd, &n);
	if (!data || n < 2) {
		ESP_LOGE(TAG, "pump_targets: data %p length %d", data, n);
		return 0;
	}
	int step = fam <= 12 ? 2 : 3;
	int num = data[0] - 1;
	if (step + num >= n) {
		ESP_LOGE(TAG, "pump_targets: invalid length field (%d) for %d-byte packet", num, n);
		return 0;
	}
	if (num % step != 0) {
		ESP_LOGE(TAG, "pump_targets: length field (%d) not divisible by %d", num, step);
		return 0;
	}
	glucose_units_t units = data[1];
	data += 2;
	int count = 0;
	for (int i = 0; i <= num - step && count < max; i += step, count++, r++) {
		int t = data[i];
		if (t == 0 && count != 0) {
			break;
		}
		r->start = half_hours(t);
		r->units = units;
		r->low = int_to_glucose(data[i + 1], units);
		r->high = fam > 12 ? int_to_glucose(data[i + 2], units) : r->low;
	}
	return count;
}

int pump_temp_basal(int *minutes) {
	int n;
	uint8_t *data = short_command(CMD_TEMP_BASAL, &n);
	if (!data || n < 7 || data[0] != 6) {
		return -1;
	}
	*minutes = two_byte_be_int(&data[5]);
	if (data[1] != 0) {
		ESP_LOGE(TAG, "pump_temp_basal: unsupported %d percent rate", data[2]);
		return 0;
	}
	return two_byte_be_int(&data[3]) * 25;
}
