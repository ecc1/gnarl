#include <driver/adc.h>
#include <esp_adc_cal.h>

#include "voltage.h"

// Value of voltage divider resistor R1, between VBAT and ADC input, in kΩ.
#define VDIV_R1		10000

// Value of voltage divider resistor R2, between ADC input and GND, in kΩ.
#define VDIV_R2		2000

#define ADC_PIN		ADC1_CHANNEL_7	// GPIO pin 35
#define DEFAULT_VREF	1100
#define NUM_SAMPLES	64

static esp_adc_cal_characteristics_t *adc_chars;

void voltage_init(void) {
#ifdef BROKEN
	adc1_config_width(ADC_WIDTH_BIT_12);
	// 0 dB attenuation = 0 to 1.1V voltage range
	adc1_config_channel_atten(ADC_PIN, ADC_ATTEN_DB_0);
	adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_value_t cal = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_0, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
	const char *cal_type;
	switch (cal) {
	case ESP_ADC_CAL_VAL_EFUSE_VREF:
		cal_type = "eFuse Vref";
		break;
	case ESP_ADC_CAL_VAL_EFUSE_TP:
		cal_type = "eFuse two point";
		break;
	default:
		cal_type = "default";
		break;
	}
	ESP_LOGI(TAG, "ADC calibration: %s", cal_type);
#endif // BROKEN
}

int voltage_read(int *rawp) {
	int raw = 0;
	for (int i = 0; i < NUM_SAMPLES; i++) {
		raw += adc1_get_raw(ADC_PIN);
	}
	raw /= NUM_SAMPLES;
	int voltage = esp_adc_cal_raw_to_voltage(raw, adc_chars) * (VDIV_R1 + VDIV_R2) / VDIV_R2;
	if (rawp) {
		*rawp = raw;
	}
	return voltage;
}

char *voltage_string(int mV, char *buf) {
	char *p = buf;
	if (mV < 0) {
		*p++ = '-';
		mV = -mV;
	}
	int v = mV / 1000;
	int cv = (mV % 1000 + 5) / 10; // centivolts
	sprintf(p, "%d.%02d V", v, cv);
	return buf;
}

// Voltage may exceed these bounds while charging from USB.
#define MIN_BAT	3300
#define MAX_BAT	4000

int voltage_level(int mV) {
	if (mV <= MIN_BAT) {
		return 0;
	}
	if (mV >= MAX_BAT) {
		return 100;
	}
	return 100 * (mV - MIN_BAT) / (MAX_BAT - MIN_BAT);
}
