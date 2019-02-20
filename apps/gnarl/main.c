#include <Arduino.h>

#include "display.h"
#include "gnarl.h"
#include "rfm95.h"

#define PUMP_FREQUENCY 916600000

void app_main() {
	initArduino();

	rfm95_init();
	uint8_t v = read_version();
	ESP_LOGD(TAG, "radio version %d.%d", version_major(v), version_minor(v));
	set_frequency(PUMP_FREQUENCY);
	ESP_LOGD(TAG, "frequency set to %d Hz", read_frequency());

	gnarl_init();

	display_init();
}
