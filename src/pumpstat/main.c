#include <unistd.h>

#include <esp_sleep.h>

#include "medtronic.h"
#include "module.h"
#include "oled.h"
#include "pump_config.h"
#include "rfm95.h"

int basal_rate, basal_minutes;
int reservoir_level;
int battery_level;
int model;

void get_pump_info() {
	printf("waking pump %s\n", PUMP_ID);
	if (!pump_wakeup()) {
		printf("wakeup failed\n");
		model = -1;
		return;
	}
	model = pump_model();
	printf("model %d\n", model);
	battery_level = pump_battery();
	printf("battery %d\n", battery_level);
	reservoir_level = pump_reservoir();
	printf("reservoir %d\n", reservoir_level);
	basal_rate = pump_temp_basal(&basal_minutes);
	printf("temp basal %d for %d min\n", basal_rate, basal_minutes);
}

char str[100];

#define FP1(n)	(n)/1000, ((n)%1000)/100
#define FP2(n)	(n)/1000, ((n)%1000)/10

void display_info() {
	oled_clear();
	oled_font_medium();
	if (model == -1) {
		oled_align_center();
		oled_draw_string(64, 30, "Unable to");
		oled_draw_string(64, 50, "wake up pump");
		oled_update();
		return;
	}
	oled_align_left();
	sprintf(str, "%d.%d U/hr  %d min", FP1(basal_rate), basal_minutes);
	oled_draw_string(0, 20, str);
	sprintf(str, "%d.%dU  %d.%02dV", FP1(reservoir_level), FP2(battery_level));
	oled_draw_string(0, 40, str);
	sprintf(str, "model %d pump", model);
	oled_draw_string(0, 60, str);
	oled_update();
}

void splash() {
	oled_on();
	oled_font_large();
	oled_align_center();
	oled_draw_string(64, 44, "Hello");
	oled_update();
}

#define SECONDS		1000000
#define DISPLAY_TIMEOUT	(10*SECONDS)

void app_main() {
	oled_init();
	splash();
	pump_set_id(PUMP_ID);
	rfm95_init();
	set_frequency(PUMP_FREQUENCY);
	printf("frequency set to %d Hz\n", read_frequency());
	get_pump_info();
	display_info();
	usleep(DISPLAY_TIMEOUT);
	// Wake up on button press.
	esp_sleep_enable_ext0_wakeup(BUTTON, 0);
	esp_deep_sleep_start();
}
