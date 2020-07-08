#include <driver/adc.h>
#include <esp_adc_cal.h>

#include "voltage.h"

#define VBAT		ADC1_CHANNEL_7	// GPIO pin 35
#define DEFAULT_VREF	1100
#define NUM_SAMPLES	64

static esp_adc_cal_characteristics_t *adc_chars;

void voltage_init(void) {
	adc1_config_width(ADC_WIDTH_BIT_12);
	// 0 dB attenuation = 0 to 1.1V voltage range
	adc1_config_channel_atten(VBAT, ADC_ATTEN_DB_0);
	adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_0, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
}

int voltage_read(int *rawp) {
	int raw = 0;
	for (int i = 0; i < NUM_SAMPLES; i++) {
		raw += adc1_get_raw(VBAT);
	}
	raw /= NUM_SAMPLES;
	// With 10M + 2M voltage divider, ADC voltage is 1/6 of actual value.
	int voltage = esp_adc_cal_raw_to_voltage(raw, adc_chars) * 6;
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
