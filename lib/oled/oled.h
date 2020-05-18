#ifndef _OLED_H
#define _OLED_H

#include <stdint.h>

#define OLED_WIDTH	128
#define OLED_HEIGHT	64

void oled_init(void);

void oled_on(void);
void oled_off(void);

void oled_clear(void);
void oled_update(void);

int oled_font_width(void);
int oled_font_ascent(void);
int oled_font_descent(void);

void oled_font_small(void);
void oled_font_medium(void);
void oled_font_large(void);

void oled_font_sans_serif(void);
void oled_font_serif(void);
void oled_font_monospace(void);

void oled_align_left(void);
void oled_align_right(void);
void oled_align_center(void);
void oled_align_center_both(void);

int oled_string_width(const char *s);
void oled_draw_string(int x, int y, const char *s);

void oled_draw_box(int x, int y, int w, int h);
void oled_draw_xbm(int x, int y, int w, int h, const uint8_t *bitmap);

void oled_brightness(uint8_t value);

#endif // _OLED_H
