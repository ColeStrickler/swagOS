#ifndef VIDEO_H
#define VIDEO_H
#include <font.h>

#define RGB_COLOR(r, g, b) (uint32_t)((b) | (g << 8) | (r << 16))
void draw_character(int x, int y, uint32_t color, enum FONT_BITMAP_KEY key);

void draw_cursor_loc();


void set_pixel(int x, int y, int color);
void clear_screen(uint8_t r, uint8_t g, uint8_t b);
void draw_rect(uint32_t top_left_x, uint32_t top_left_y, uint32_t width, uint32_t height, uint8_t r, uint8_t g, uint8_t b);
void video_init();

#endif

