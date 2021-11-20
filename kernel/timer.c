#include <timer.h>
#include <kernel.h>
#include <string.h>
#include <console.h>
#include <interrupt.h>
#include <text.h>
#include <thread.h>

static uint32_t tick_val = 0;
static const uint32_t tick_frequency = KERN_TICK_HZ;

static uint32_t tick_to_millisecond()
{
    uint32_t current_tick = tick_val;
    uint32_t current_millisecond = current_tick * (tick_frequency / 10);

    return current_millisecond;
}

static void timer_isr(struct registers *reg)
{
    ++tick_val;

    if (tick_val % KERNEL_THREAD_STEP == 0)
    {
        thread_schedule();
    }
}

void init_timer()
{
    /* Intel 8253/8254 每秒中断次数 */
    uint32_t divisor = (CLOCK_TICK_RATE + tick_frequency / 2) / tick_frequency;

    register_interrupt(IRQ_0, timer_isr);
    enable_interrupt(IRQ_0);

    /*
     *  0011 0110
     *  时钟中断由Intel 8253/8254产生，因此不设置为0（8086模式）
     *  端口地址范围是40h~43h
     */
    io_out8(0x43, 0x36);

    io_out8(0x40, (uint8_t)(divisor & 0xff));           /* 设置低位 */
    io_out8(0x40, (uint8_t)((divisor >> 8) & 0xff));    /* 设置高位 */

    LOG("cmos tick frequency = %dHZ\n", tick_frequency);
}

void delay(uint32_t millisecond)
{
    uint32_t current_tick = tick_to_millisecond();

    while ((tick_to_millisecond() - current_tick) < millisecond);
}

void print_tick()
{
    uint32_t current_tick = tick_to_millisecond();
    uint32_t second = current_tick / 1000;
    uint32_t msecond = current_tick - second * 1000;
    size_t tick_str_len = 12;

    tick_str_len -= strlen(to_dec_string(second));
    tick_str_len -= strlen(to_dec_string(msecond));

    if (msecond < 100)
    {
        if (msecond < 10)
        {
            --tick_str_len;
        }
        --tick_str_len;
    }

    console_out("[", 0, 0);
    while (tick_str_len --> 0)
    {
        console_out(" ", 0, 0);
    }
    printk("%d.%s%s%d", second, &"0"[!!(msecond / 100)], &"0"[!!(msecond / 10)], msecond);
    console_out("]", 0, 0);
}
