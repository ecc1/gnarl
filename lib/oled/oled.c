#include "module.h"
#include "oled.h"

#ifdef OLED_ENABLE

#include <u8g2.h>

static u8g2_t u8g2;

static enum {
	SANS_SERIF,
	SERIF,
	MONOSPACE,
} font_style = SANS_SERIF;
#define NUM_STYLES	(MONOSPACE + 1)

void oled_font_sans_serif(void) {
	font_style = SANS_SERIF;
}

void oled_font_serif(void) {
	font_style = SERIF;
}

void oled_font_monospace(void) {
	font_style = MONOSPACE;
}

static enum {
	SMALL,
	MEDIUM,
	LARGE,
} font_size = MEDIUM;
#define NUM_SIZES	(LARGE + 1)

void oled_font_small(void) {
	font_size = SMALL;
}

void oled_font_medium(void) {
	font_size = MEDIUM;
}

void oled_font_large(void) {
	font_size = LARGE;
}

static const uint8_t *font[NUM_STYLES][NUM_SIZES] = {
	// sans-serif
	{
		u8g2_font_helvR08_tr,
		u8g2_font_helvR12_tr,
		u8g2_font_helvR24_tr,
	},
	// serif
	{
		u8g2_font_lubR08_tr,
		u8g2_font_lubR12_tr,
		u8g2_font_lubR24_tr,
	},
	// monospace
	{
		u8g2_font_courR08_tr,
		u8g2_font_courR12_tr,
		u8g2_font_courR24_tr,
	},
};

static void set_font(void) {
	u8g2_SetFont(&u8g2, font[font_style][font_size]);
}

void oled_on(void) {
	u8g2_SetPowerSave(&u8g2, 0);
}

void oled_off(void) {
	u8g2_SetPowerSave(&u8g2, 1);
}

void oled_clear(void) {
	u8g2_ClearBuffer(&u8g2);
}

void oled_update(void) {
	u8g2_SendBuffer(&u8g2);
}

int oled_font_width(void) {
	set_font();
	return u8g2_GetMaxCharWidth(&u8g2);
}

int oled_font_ascent(void) {
	set_font();
	return u8g2_GetAscent(&u8g2);
}

int oled_font_descent(void) {
	set_font();
	return -u8g2_GetDescent(&u8g2);
}

static enum {
	LEFT,
	RIGHT,
	CENTER,
} align_mode = LEFT;

void oled_align_left(void) {
	align_mode = LEFT;
}

void oled_align_right(void) {
	align_mode = RIGHT;
}

void oled_align_center(void) {
	align_mode = CENTER;
}

int oled_string_width(const char *s) {
	set_font();
	return u8g2_GetStrWidth(&u8g2, s);
}

void oled_draw_string(int x, int y, const char *s) {
	set_font();
	if (align_mode != LEFT) {
		int w = u8g2_GetStrWidth(&u8g2, s);
		if (align_mode == CENTER) {
			w /= 2;
		}
		x -= w;
	}
	u8g2_DrawStr(&u8g2, x, y, s);
}

void oled_draw_box(int x, int y, int w, int h) {
	u8g2_DrawBox(&u8g2, x, y, w, h);
}

void oled_draw_xbm(int x, int y, int w, int h, const uint8_t *bitmap) {
	u8g2_DrawXBM(&u8g2, x, y, w, h, bitmap);
}

void oled_brightness(uint8_t value) {
	u8g2_SetContrast(&u8g2, value);
}

uint8_t i2c_callback(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t gpio_and_delay_callback(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

void oled_init(void) {
	u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, i2c_callback, gpio_and_delay_callback);
	u8x8_SetI2CAddress(&u8g2.u8x8, 0x3C << 1);
	u8g2_InitDisplay(&u8g2);

	oled_on();
	oled_clear();
}

#else // !OLED_ENABLE

void oled_init() {}

void oled_on() {}
void oled_off() {}

void oled_clear() {}
void oled_update() {}

int oled_font_width() { return 0; }
int oled_font_ascent() { return 0; }
int oled_font_descent() { return 0; }

void oled_font_small() {}
void oled_font_medium() {}
void oled_font_large() {}

void oled_font_sans_serif() {}
void oled_font_serif() {}
void oled_font_monospace() {}

void oled_align_left() {}
void oled_align_right() {}
void oled_align_center() {}
void oled_align_center_both() {}

int oled_string_width(const char *s) { return 0; }
void oled_draw_string(int x, int y, const char *s) {}

void oled_draw_box(int x, int y, int w, int h) {}

void oled_draw_xbm(int x, int y, int w, int h, const uint8_t *bitmap) {}

void oled_brightness(uint8_t value) {}

#endif // !OLED_ENABLE
