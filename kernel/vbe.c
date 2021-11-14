#include <vbe.h>
#include "fonts.h"

static uint32_t screen_width;
static uint32_t screen_height;
static struct vbe_info vbe;
static uint32_t screen_length;
static uint8_t colors[6] = {0};

void init_vbe()
{
    vbe.color_number = *((uint16_t *)VBE_ADDR);
    vbe.width        =  (uint32_t)(*((uint16_t *)(VBE_ADDR + 2)));
    vbe.height       =  (uint32_t)(*((uint16_t *)(VBE_ADDR + 4)));
    vbe.vram         =  (uint8_t *)(*((uint32_t *)(VBE_ADDR + 6)));

    screen_width     = vbe.width;
    screen_height    = vbe.height;
    screen_length    = vbe.width * PIXEL_BYTE;
}

uint32_t get_screen_width()
{
    return screen_width;
}

uint32_t get_screen_height()
{
    return screen_height;
}

void put_pixel(int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b)
{
    if (x >= vbe.width || y >= vbe.height)
    {
        return;
    }

    int pos = y * screen_length + x * PIXEL_BYTE;
    vbe.vram[pos + 0] = b;
    vbe.vram[pos + 1] = g;
    vbe.vram[pos + 2] = r;
}

void put_dword_pixels(int32_t x, int32_t y, int32_t width, int32_t height, uint8_t r, uint8_t g, uint8_t b)
{
    struct color_bytes color =
    {
#if __BYTE_ORDER__ ==__ORDER_BIG_ENDIAN__
        {r, g, b}
#else
        {b, g, r}
#endif
    };
    struct color_bytes *pixel_start = (struct color_bytes *)&vbe.vram[y * screen_length + x * PIXEL_BYTE];
    struct color_bytes *pixel_end = (struct color_bytes *)&vbe.vram[(y + height) * screen_length + (x + width) * PIXEL_BYTE];

    while (pixel_start < pixel_end)
    {
        *pixel_start = color;
        ++pixel_start;
    }
}

void send_pixel(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    if ((x1 >= vbe.width || y1 >= vbe.height) ||
        (x2 >= vbe.width || y2 >= vbe.height))
    {
        return;
    }

    int pos1 = y1 * screen_length + x1 * PIXEL_BYTE;
    int pos2 = y2 * screen_length + x2 * PIXEL_BYTE;
    vbe.vram[pos2 + 0] = vbe.vram[pos1 + 0];
    vbe.vram[pos2 + 1] = vbe.vram[pos1 + 1];
    vbe.vram[pos2 + 2] = vbe.vram[pos1 + 2];
}

void send_dword_pixels(int32_t x1, int32_t y1, int32_t width, int32_t height, int32_t x2, int32_t y2)
{
    uint32_t *pixel_start = (uint32_t *)&vbe.vram[y1 * screen_length + x1 * PIXEL_BYTE];
    uint32_t *pixel_end = (uint32_t *)&vbe.vram[(y1 + height) * screen_length + (x1 + width) * PIXEL_BYTE];
    uint32_t *pixel_target = (uint32_t *)&vbe.vram[y2 * screen_length + x2 * PIXEL_BYTE];

    while (pixel_start < pixel_end)
    {
        *pixel_target = *pixel_start;
        ++pixel_target;
        ++pixel_start;
    }
}

void set_color(uint32_t color, uint32_t background)
{
    colors[0] = RED(color);
    colors[1] = GREEN(color);
    colors[2] = BLUE(color);
    colors[3] = RED(background);
    colors[4] = GREEN(background);
    colors[5] = BLUE(background);
}

void set_color_invert()
{
    colors[2] ^= colors[5];
    colors[5] ^= colors[2];
    colors[2] ^= colors[5];

    colors[1] ^= colors[4];
    colors[4] ^= colors[1];
    colors[1] ^= colors[4];

    colors[0] ^= colors[3];
    colors[3] ^= colors[0];
    colors[0] ^= colors[3];
}

void put_char(uint8_t n, int32_t x, int32_t y)
{
    int start_x = x, start_y = y;
    int end_x = x + FONT_W, end_y = y + FONT_H;
    int line = 0;
    char *font = (char *)ascii_fonts[n < 32 || n > 126 ? 95 : n - 32];

    for (; start_y < end_y; ++start_y, ++line)
    {
        for (; start_x < end_x; ++start_x)
        {
            if (((font[line] >> (end_x - start_x - 1)) & 1))
            {
                put_pixel(start_x, start_y, colors[0], colors[1], colors[2]);
            }
            else
            {
                put_pixel(start_x, start_y, colors[3], colors[4], colors[5]);
            }
        }
        start_x -= FONT_W;
    }
}

void put_line(int x1, int y1, int x2, int y2)
{
    int dx, dy, x, y, p, k;

    dx = x2 - x1;
    dy = y2 - y1;
    if (dx < 0)
    {
        dx = -dx;
    }
    if (dy < 0)
    {
        dy = -dy;
    }
    k  = (x2 - x1) * (y2 - y1);

    if (dx >= dy)
    {
        p = (dy << 1) - dx;
        if (x1 < x2)
        {
            x = x1;
            y = y1;
        }
        else
        {
            y = y2;
            x = x2;
            x2 = x1;
        }

        while (x < x2)
        {
            put_pixel(x, y, colors[0], colors[1], colors[2]);
            ++x;

            if (p < 0)
            {
                p += dy << 1;
            }
            else
            {
                if (k > 0)
                {
                    ++y;
                }
                else
                {
                    --y;
                }

                p += (dy - dx) << 1;
            }
        }
    }
    else
    {
        p = (dx << 1) - dy;
        if (y1 < y2)
        {
            x = x1;
            y = y1;
        }
        else
        {
            y = y2;
            x = x2;
            y2 = y1;
        }

        while (y < y2)
        {
            put_pixel(x, y, colors[0], colors[1], colors[2]);
            ++y;

            if (p < 0)
            {
                p += dx << 1;
            } else
            {
                if (k > 0)
                {
                    ++x;
                }
                else
                {
                    --x;
                }

                p += (dx - dy) << 1;
            }
        }
    }
}

void put_rect(int x, int y, int width, int height)
{
    int set_x, set_y;

    for (set_y = y + height - 1; set_y >= y; --set_y)
    {
        for (set_x = x + width - 1; set_x >= x; set_x--)
        {
            put_pixel(set_x, set_y, colors[0], colors[1], colors[2]);
        }
    }
}

void put_frame(int x, int y, int width, int height)
{
    int set_x, set_y, y_tmp = y + height;

    for (set_x = x + width - 1; set_x >= x; --set_x)
    {
        put_pixel(set_x, y, colors[0], colors[1], colors[2]);
        put_pixel(set_x, y_tmp, colors[0], colors[1], colors[2]);
    }

    for (set_y = y + height - 1, width += x - 1; set_y > y; --set_y)
    {
        put_pixel(x, set_y, colors[0], colors[1], colors[2]);
        put_pixel(width, set_y, colors[0], colors[1], colors[2]);
    }
}

void put_circle(int x, int y, int radius)
{
    int set_x, set_y, range_x = x + radius - 1, range_y = radius - 1, x_tmp = (x << 1) - 1, y_tmp = (y << 1) - 1;

    for (set_x = x; set_x <= range_x; ++set_x)
    {
        for (; radius * radius < range_y * range_y + (set_x - x) * (set_x - x); --range_y);
        for (set_y = range_y + y; set_y >= y; --set_y)
        {
            put_pixel(x_tmp - set_x, y_tmp - set_y, colors[0], colors[1], colors[2]);
            put_pixel(        set_x, y_tmp - set_y, colors[0], colors[1], colors[2]);
            put_pixel(x_tmp - set_x,         set_y, colors[0], colors[1], colors[2]);
            put_pixel(        set_x,         set_y, colors[0], colors[1], colors[2]);
        }
    }
}

void put_ring(int x, int y, int radius)
{
    int set_x = 0, set_y = radius, p = 1 - radius;

    while (set_x < set_y)
    {
        put_pixel(x + set_x, y + set_y, colors[0], colors[1], colors[2]);
        put_pixel(x - set_x, y + set_y, colors[0], colors[1], colors[2]);
        put_pixel(x - set_x, y - set_y, colors[0], colors[1], colors[2]);
        put_pixel(x + set_x, y - set_y, colors[0], colors[1], colors[2]);
        put_pixel(x + set_y, y + set_x, colors[0], colors[1], colors[2]);
        put_pixel(x - set_y, y + set_x, colors[0], colors[1], colors[2]);
        put_pixel(x - set_y, y - set_x, colors[0], colors[1], colors[2]);
        put_pixel(x + set_y, y - set_x, colors[0], colors[1], colors[2]);

        ++set_x;

        if (p < 0)
        {
            p += (set_x << 1) + 1;
        }
        else
        {
            --set_y;
            p += ((set_x - set_y) << 1) + 1;
        }
    }
}
