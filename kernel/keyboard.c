#include <interrupt.h>
#include <io.h>
#include <kernel.h>
#include <types.h>

#include "keymap.h"

#define KBSTATP         0x64    /* 键盘状态 */
#define KBS_DIB         0x01    /* 键盘在缓冲区中的数据 */
#define KBDATAP         0x60    /* 键盘在IO口中的数据 */

#define KEY_BUF_SIZE    32

static uint8_t chars_fifo[KEY_BUF_SIZE];
static uint8_t cursor_in = 0, cursor_out = 1;

static void keyboard_isr()
{
    static uint32_t shift;
    static uint8_t *charcode[4] =
    {
        normalmap,
        shiftmap,
        ctlmap,
        ctlmap
    };
    uint32_t st, data, c;

    st = io_in8(KBSTATP);
    if((st & KBS_DIB) == 0)
    {
        return;
    }
    data = io_in8(KBDATAP);

    if (data == 0xE0) {
        shift |= E0ESC;
        return;
    }
    else if(data & 0x80)
    {
        data = (shift & E0ESC ? data : data & 0x7F);
        shift &= ~(shiftcode[data] | E0ESC);
        return;
    }
    else if(shift & E0ESC)
    {
        data |= 0x80;
        shift &= ~E0ESC;
    }

    shift |= shiftcode[data];
    shift ^= togglecode[data];
    c = charcode[shift & (CTL | SHIFT)][data];

    if (shift & CAPSLOCK)
    {
        if ('a' <= c && c <= 'z')
        {
            c += 'A' - 'a';
        }
        else if('A' <= c && c <= 'Z')
        {
            c += 'a' - 'A';
        }
    }

    chars_fifo[cursor_in++] = c;
    cursor_in %= KEY_BUF_SIZE;
}

uint8_t get_key()
{
    char ch;
    cursor_out %= KEY_BUF_SIZE;
    ch = chars_fifo[cursor_out];
    chars_fifo[cursor_out++] = 0;

    return ch;
}

void init_keyboard(void)
{
    register_interrupt(IRQ_1, keyboard_isr);
    enable_interrupt(IRQ_1);

    LOG("ps2 keyboard hook input\n");
}
