#include <stdio.h>
#include <unistd.h>

#include "oled.h"
#include "voltage.h"

void display_voltage(int mV, int raw) {
	oled_clear();
	oled_font_large();
	oled_align_center();
	char buf[40];
	voltage_string(mV, buf);
	oled_draw_string(OLED_WIDTH/2, 30, buf);
	oled_font_small();
	sprintf(buf, "%d%%", voltage_level(mV));
	oled_draw_string(OLED_WIDTH/2, 45, buf);
	sprintf(buf, "raw:  %4d    0x%03X", raw, raw);
	oled_draw_string(OLED_WIDTH/2, 60, buf);
	oled_update();
}

void app_main(void) {
	voltage_init();
	oled_init();
	oled_brightness(1);
	for (;;) {
		int mV, raw;
		mV = voltage_read(&raw);
		display_voltage(mV, raw);
		usleep(1e6);
	}
}
