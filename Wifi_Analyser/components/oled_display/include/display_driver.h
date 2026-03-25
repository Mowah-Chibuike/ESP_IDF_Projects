#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "font.h"

#define DD_WIDTH        128
#define DD_HEIGHT       64
#define DD_COLUMNS      DD_WIDTH
#define DD_PAGES        (DD_HEIGHT / 8)


extern uint8_t DD_FRAMEBUFFER[DD_PAGES][DD_COLUMNS];

void dd_set_pixel(int x, int y, bool state);
uint8_t get_pixel(int x, int y);
void dd_clear();
void dd_draw_bitmap(int x, int y, int width, int height, const uint8_t *bitmap);
void dd_draw_hline(int y);
void dd_draw_rectangle(int x, int y, int width, int height);
void dd_render_text(int x, int y, font_size_t size, char *text);
void print_framebuffer();
void update_oled();


#endif /* DISPLAY_DRIVER_H */