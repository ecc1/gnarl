#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "medtronic.h"
#include "module.h"
#include "oled.h"
#include "pump_config.h"
#include "rfm95.h"

time_t pump_time;

void get_time() {
	printf("waking pump %s\n", PUMP_ID);
	if (!pump_wakeup()) {
		printf("pump_wakeup() failed\n");
		return;
	}
	pump_time = pump_clock();
	if (pump_time == -1) {
		printf("pump_clock() failed\n");
		return;
	}
	struct timeval tv = { .tv_sec = pump_time };
	settimeofday(&tv, 0);
}

char str[100];

void display_info() {
	time_t t = time(0);
	struct tm *tm = localtime(&t);
	oled_clear();
	oled_font_medium();
	oled_font_monospace();
	oled_align_center();
	strftime(str, sizeof(str), "%F", tm);
	oled_draw_string(64, 25, str);
	strftime(str, sizeof(str), "%T", tm);
	oled_draw_string(64, 55, str);
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

void app_main() {
	oled_init();
	splash();
	pump_set_id(PUMP_ID);
	rfm95_init();
	set_frequency(PUMP_FREQUENCY);
	printf("frequency set to %d Hz\n", read_frequency());
	get_time();
	for (;;) {
		display_info();
		usleep(1*SECONDS);
	}
}
