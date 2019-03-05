#include <pins_arduino.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include <SSD1306.h>
#pragma GCC diagnostic pop

extern "C" {
#include "oled.h"
}

static SSD1306 d(0x3C, OLED_SDA, OLED_SCL);

void oled_init() {
	pinMode(OLED_RST, OUTPUT);
	digitalWrite(OLED_RST, LOW);
	delay(50);
	digitalWrite(OLED_RST, HIGH);

	d.init();
	d.flipScreenVertically();
}

void oled_on() {
	d.displayOn();
}

void oled_off() {
	d.displayOff();
}

void oled_clear() {
	// d.clear() leaves artifacts
	d.resetDisplay();
}

void oled_update() {
	d.display();
}

void font_small() {
	d.setFont(ArialMT_Plain_10);
}

void font_medium() {
	d.setFont(ArialMT_Plain_16);
}

void font_large() {
	d.setFont(ArialMT_Plain_24);
}

void align_left() {
	d.setTextAlignment(TEXT_ALIGN_LEFT);
}

void align_right() {
	d.setTextAlignment(TEXT_ALIGN_RIGHT);
}

void align_center() {
	d.setTextAlignment(TEXT_ALIGN_CENTER);
}

void align_center_both() {
	d.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
}

void draw_string(int x, int y, const char *s) {
	d.drawString(x, y, s);
}
