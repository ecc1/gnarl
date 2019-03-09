extern void oled_init();

extern void oled_on();
extern void oled_off();

extern void oled_clear();
extern void oled_update();

extern void font_small();
extern void font_medium();
extern void font_large();

extern void align_left();
extern void align_right();
extern void align_center();
extern void align_center_both();

extern void draw_string(int x, int y, const char *s);
extern void draw_freq_bar(uint freq, int rssi);
