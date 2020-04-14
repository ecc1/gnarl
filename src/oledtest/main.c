#include <stdio.h>
#include <unistd.h>

#include "oled.h"

const char *lorem_ipsum = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";

inline int is_space(char c) {
	return c == ' ' || c == '\t' || c == '\n';
}

const char *next_word(const char *s, char *buf, int bufsize) {
	int n = 0;
	while (*s != 0 && is_space(*s)) {
		s++;
	}
	if (*s == 0) {
		return 0;
	}
	while (*s != 0 && !is_space(*s)) {
		if (n == bufsize - 1) {
			break;
		}
		buf[n++] = *s++;
	}
	buf[n] = 0;
	return s;
}

void draw_text(const char *s) {
	int asc = oled_font_ascent();
	int desc = oled_font_descent();
	int vsp = 1;
	int hsp = oled_string_width(" ");
	int x = 0;
	int y = asc;
	char word[20];
	const char *p = s;
	for (;;) {
		p = next_word(p, word, sizeof(word));
		if (p == 0) {
			return;
		}
		int n = oled_string_width(word);
		if (x + n >= OLED_WIDTH) {
			x = 0;
			y += desc + vsp + asc;
		}
		if (y >= OLED_HEIGHT) {
			return;
		}
		printf("(%3d,%3d) [%s]\n", x, y, word);
		oled_draw_string(x, y, word);
		x += n + hsp;
	}
}

typedef void (*void_fn_t)(void);

void_fn_t font_size[] = {
	oled_font_small,
	oled_font_medium,
	oled_font_large,
};
#define NUM_SIZES	(sizeof(font_size)/sizeof(font_size[0]))

void_fn_t font_style[] = {
	oled_font_sans_serif,
	oled_font_serif,
	oled_font_monospace,
};
#define NUM_STYLES	(sizeof(font_style)/sizeof(font_style[0]))

void display(void) {
	oled_clear();
	draw_text(lorem_ipsum);
	oled_update();
	sleep(2);
}

void app_main(void) {
	oled_init();
	for (;;) {
		for (int i = 0; i < NUM_SIZES; i++) {
			font_size[i]();
			for (int j = 0; j < NUM_STYLES; j++) {
				font_style[j]();
				display();
			}
		}
	}
}
