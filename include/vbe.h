#ifndef _VBE_H_
#define _VBE_H_

#include <types.h>
#include <memory.h>

#define VBE_ADDR    (KERNEL_MAP_BASE_VADDR + 0x70000)
#define PIXEL_BYTE  3

struct vbe_info
{
    uint16_t color_number;
    uint32_t width;
    uint32_t height;
    uint8_t  *vram;
};

struct color_bytes
{
    uint8_t byte[PIXEL_BYTE];
};

#define RED(x)      (((x) >> 24) & 0xff)
#define GREEN(x)    (((x) >> 16) & 0xff)
#define BLUE(x)     (((x) >>  8) & 0xff)
#define ALPHA(x)    (((x) & 0xff))

#define FONT_W 8
#define FONT_H 16

void init_vbe();

uint32_t get_screen_width();
uint32_t get_screen_height();

void put_pixel(int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b);
void put_dword_pixels(int32_t x, int32_t y, int32_t width, int32_t height, uint8_t r, uint8_t g, uint8_t b);
void send_pixel(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
void send_dword_pixels(int32_t x1, int32_t y1, int32_t width, int32_t height, int32_t x2, int32_t y2);
void set_color(uint32_t color, uint32_t background);
void set_color_invert();

void put_char(uint8_t n, int32_t x, int32_t y);
void put_line(int x1, int y1, int x2, int y2);
void put_rect(int x, int y, int width, int height);
void put_frame(int x, int y, int width, int height);
void put_circle(int x, int y, int radius);
void put_ring(int x, int y, int radius);

#endif
