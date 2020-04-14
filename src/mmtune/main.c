#include <unistd.h>

#include <esp_sleep.h>

#include "medtronic.h"
#include "module.h"
#include "oled.h"
#include "pump_config.h"
#include "rfm95.h"

int model;
int rssi;

int try_frequency(uint32_t frequency) {
	set_frequency(frequency);
	printf("frequency set to %d Hz\n", read_frequency());
	printf("waking pump %s\n", PUMP_ID);
	if (model == 0 && !pump_wakeup()) {
		printf("wakeup failed\n");
		return -128;
	}
	model = pump_model();
	printf("model %d\n", model);
	rssi = read_rssi();
	printf("rssi %d\n", rssi);
	return rssi;
}

void splash(void) {
	oled_on();
	oled_font_large();
	oled_align_center();
	oled_draw_string(64, 44, "MMTune");
	oled_update();
}

void draw_freq_bar(uint32_t freq, int rssi) {
	int x = 2 + 10*freq;
	int y = -rssi/2 - 3;
	int w = 6;
	int h = OLED_HEIGHT - y;
	oled_draw_box(x, y, w, h);
}

#define SECONDS		1000000
#define DISPLAY_TIMEOUT (30 * SECONDS)

#define FP_FQ(n)	(n)/1000000, ((n)%1000000)/1000

char str[100];

void app_main(void) {
	rfm95_init();
	uint8_t v = read_version();
	printf("radio version %d.%d\n", version_major(v), version_minor(v));

	oled_init();
	splash();
	usleep(2 * SECONDS);
	pump_set_id(PUMP_ID);

	oled_clear();
	oled_font_medium();
	oled_align_left();
	oled_draw_string(0, 20, "Scanning...");
	oled_update();
	usleep(1 * SECONDS);

	int best_rssi = -128;
	uint32_t best_freq = 0;
	for (int i = 0; i <= 12; i++) {
		uint32_t cur_freq = MMTUNE_START + 50000*i;
		int rssi = try_frequency(cur_freq);
		if (rssi > best_rssi) {
			best_rssi = rssi;
			best_freq = cur_freq;
		}
		draw_freq_bar(i, rssi);
		usleep(100);
		oled_update();
		usleep(1 * SECONDS);
	}
	printf("Best frequency %d Hz\n", best_freq);
	printf("RSSI: %d\n", best_rssi);
	oled_draw_string(0, 20, "Scanning... done.");
	usleep(100);
	oled_update();
	usleep(5 * SECONDS);

	oled_clear();
	oled_font_medium();
	oled_align_left();
	sprintf(str, "Best RSSI: %d", best_rssi);
	oled_draw_string(0, 20, str);
	sprintf(str, "Best Frq: %d.%03d", FP_FQ(best_freq));
	oled_draw_string(0, 40, str);
	usleep(100);
	oled_update();

	usleep(DISPLAY_TIMEOUT);

	// Wake up on button press.
	esp_sleep_enable_ext0_wakeup(BUTTON, 0);
	esp_deep_sleep_start();
}
