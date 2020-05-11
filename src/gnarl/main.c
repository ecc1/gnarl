#include "gnarl.h"

#include <stdio.h>
#include "rfm95.h"

#define PUMP_FREQUENCY 868000000

void app_main(void) {
	ESP_LOGI(TAG, "%s", SUBG_RFSPY_VERSION);
	rfm95_init();
	uint8_t v = read_version();
	ESP_LOGD(TAG, "radio version %d.%d", version_major(v), version_minor(v));
	set_frequency(PUMP_FREQUENCY);
	ESP_LOGD(TAG, "frequency set to %d Hz", read_frequency());
	gnarl_init();
}
