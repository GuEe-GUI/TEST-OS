#include <console.h>
#include <kernel.h>
#include <vbe.h>
#include <version.h>

static struct
{
    uint32_t x, y;
    uint32_t max_x, max_y;
    uint32_t width, height;
    uint32_t fill, clear;
} console =
{
    .x = 0,
    .y = 0,
    .fill = CONSOLE_FILL,
    .clear = CONSOLE_CLEAR
};

void init_console(uint32_t width, uint32_t height)
{
    console.width = width;
    console.height = height;

    console.max_x = (console.width / FONT_W) * FONT_W;
    console.max_y = (console.height / FONT_H) * FONT_H;

    set_color(console.fill, console.clear);
    console_cls();
    console_dump_version();

    LOG("console width = %dPX, height = %dPX", console.width, console.height);
}

void console_dump_version(void)
{
    int i, j, k = 0;

    /*
     * @@@@@@..@@@@@@...@@@@...@@@@@@...@@@@....@@@@.
     * ..@@....@@......@@........@@....@@..@@..@@....
     * ..@@....@@@@.....@@@@.....@@....@@..@@...@@@@.
     * ..@@....@@..........@@....@@....@@..@@......@@
     * ..@@....@@@@@@...@@@@.....@@.....@@@@....@@@@.
     */
    const uint32_t logo[] =
    {
        0xfcfc78fc,
        0x7878c303,
        0xc33303,
        0xf07830c,
        0xc78c3003,
        0xc330330,
        0xfc783078,
        0x78000000,
    };

    for (i = 0; i < sizeof(logo) / sizeof(uint32_t); ++i)
    {
        for (j = 31; j >= 0; --j)
        {
            console_out(((logo[i] >> j) & 1) ? "@" : " ", 0, 0);
            ++k;
            if (k >= 46)
            {
                console_out("\n", 0, 0);
                k = 0;
            }
        }
    }

    printk("\nTEST OS [Version %s]\n(C) %s %s. %s License.\n\n", OS_VERSION, OS_OWNER_YEAR, OS_OWNER_NAME, OS_LICENSE);
}

uint32_t get_console_cols(void)
{
    return console.max_x / FONT_W;
}

uint32_t get_console_rows(void)
{
    return console.max_y / FONT_H;
}

static void console_cur(int flush_old, int flush_new)
{
    static uint32_t last_x = 0, last_y = 0;

    if (flush_old)
    {
        put_char(' ', last_x, last_y);
    }

    last_x = console.x;
    last_y = console.y;

    if (flush_new)
    {
        set_color_invert();
        put_char(' ', console.x, console.y);
        set_color_invert();
    }
}

void console_out(const char *string, uint32_t color, uint32_t background)
{
    int interrupt_flag;

    cli_inherit(&interrupt_flag);

    if (color ^ background)
    {
        set_color(color, background);
    }

    while (*string)
    {
        switch (*string)
        {
        case '\n':
        {
            console_roll();
            console_cur(1, 1);
            break;
        }
        case '\t':
        {
            console.x += (4 - (console.x / FONT_W) % 4) * FONT_W;
            if (console.x >= console.max_x)
            {
                console_cur(1, 0);
                console_roll();
            }
            console_cur(1, 1);
            break;
        }
        case CONSOLE_BACK_FORCE:
        {
            if (console.x != 0)
            {
                console.x -= FONT_W;
            }
            else if (console.y != 0)
            {
                console.x = console.max_x - FONT_W;
                console.y -= FONT_H;
            }
            console_cur(1, 1);
            break;
        }
        case '\b':
        {
            if (console.x != 0)
            {
                console.x -= FONT_W;
                console_cur(1, 1);
            }
            break;
        }
        case '\r':
        {
            console.x = 0;
            console_cur(1, 1);
            break;
        }
        default:
        {
            put_char(*string, console.x, console.y);
            console.x += FONT_W;
            if (console.x >= console.max_x)
            {
                console_cur(0, 0);
                console_roll();
            }
            console_cur(0, 1);
            break;
        }
        }
        string++;
    }

    sti_inherit(&interrupt_flag);
}

void console_roll(void)
{
    int interrupt_flag;

    cli_inherit(&interrupt_flag);

    console.x = 0;
    console.y += FONT_H;

    if (console.y >= console.max_y)
    {
        console_cur(1, 0);
        send_dword_pixels(0, FONT_H, console.max_x, console.max_y, 0, 0);
        put_dword_pixels(0, console.max_y - FONT_H, console.max_x, FONT_H, RED(console.clear), GREEN(console.clear), BLUE(console.clear));

        console.y -= FONT_H;
    }

    sti_inherit(&interrupt_flag);
}

void console_cls(void)
{
    int interrupt_flag;
    cli_inherit(&interrupt_flag);

    put_dword_pixels(0, 0, console.width, console.height, RED(console.clear), GREEN(console.clear), BLUE(console.clear));
    console.x = console.y = 0;

    sti_inherit(&interrupt_flag);
}
