#include <unistd.h>

#include "oled.h"

void app_main() {
	oled_init();
	font_large();
	align_center_both();
	draw_string(64, 32, "Hello!");
	oled_update();
	for (;;) {
		sleep(5);
		oled_off();
		sleep(5);
		oled_on();
	}
}
