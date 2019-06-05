#define OLED_WIDTH	128
#define OLED_HEIGHT	64

extern void oled_init();

extern void oled_on();
extern void oled_off();

extern void oled_clear();
extern void oled_update();

extern int oled_font_width();
extern int oled_font_ascent();
extern int oled_font_descent();

extern void oled_font_small();
extern void oled_font_medium();
extern void oled_font_large();

extern void oled_font_sans_serif();
extern void oled_font_serif();
extern void oled_font_monospace();

extern void oled_align_left();
extern void oled_align_right();
extern void oled_align_center();
extern void oled_align_center_both();

extern int oled_string_width(const char *s);
extern void oled_draw_string(int x, int y, const char *s);

extern void oled_draw_box(int x, int y, int w, int h);
