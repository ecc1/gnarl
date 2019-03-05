#include <Arduino.h>
#include <unistd.h>

#include "commands.h"
#include "oled.h"
#include "pump.h"
#include "rfm95.h"

#define PUMP_FREQUENCY	916600000

int basal_rate, basal_minutes;
int reservoir_level;
int battery_level;
int model;

void get_radio_info() {
	printf("waking pump %s\n", PUMP_ID);
	if (!pump_wakeup()) {
		printf("wakeup failed\n");
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
	font_medium();
	align_left();
	sprintf(str, "%d.%d U/hr  %d min", FP1(basal_rate), basal_minutes);
	draw_string(0, 0, str);
	sprintf(str, "%d.%dU  %d.%02dV", FP1(reservoir_level), FP2(battery_level));
	draw_string(0, 21, str);
	sprintf(str, "model %d pump", model);
	draw_string(0, 42, str);
	oled_update();
}

void splash() {
	oled_on();
	font_large();
	align_center_both();
	draw_string(64, 32, "Hello");
	oled_update();
}

#define SECONDS		1000000
#define DISPLAY_TIMEOUT	(5*SECONDS)

void app_main() {
	initArduino();
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);
	oled_init();
	splash();
	parse_pump_id(PUMP_ID);
	rfm95_init();
	set_frequency(PUMP_FREQUENCY);
	printf("frequency set to %d Hz\n", read_frequency());
	get_radio_info();
	display_info();
	usleep(DISPLAY_TIMEOUT);
	// Wake up on button press.
	esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, LOW);
	esp_deep_sleep_start();
}
