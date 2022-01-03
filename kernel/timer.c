#include <timer.h>
#include <kernel.h>
#include <console.h>
#include <interrupt.h>
#include <text.h>
#include <thread.h>

static volatile uint32_t tick_val = 0;
static const uint32_t tick_frequency = KERNEL_TICK_HZ;

static inline uint32_t tick_to_millisecond()
{
    return tick_val * (tick_frequency / 10);
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
    uint32_t divisor = CLOCK_TICK_RATE / tick_frequency;

    interrupt_register(IRQ_0, timer_isr);
    interrupt_enable(IRQ_0);

    /*
     *  0011 0110
     *  时钟中断由Intel 8253/8254产生，因此不设置为0（8086模式）
     *  端口地址范围是40h~43h
     */
    io_out8(0x43, 0x36);

    io_out8(0x40, (uint8_t)(divisor & 0xff));           /* 设置低位 */
    io_out8(0x40, (uint8_t)((divisor >> 8) & 0xff));    /* 设置高位 */

    LOG("cmos tick frequency = %dHZ", tick_frequency);
}

void delay(uint32_t millisecond)
{
    uint32_t current_tick = tick_to_millisecond();

    while ((tick_to_millisecond() - current_tick) < millisecond);
}

void sleep(uint32_t millisecond)
{
    /* 延时一个至少时间片以上使用该方法sleep才有意义 */
    if (millisecond > KERNEL_TICK_HZ / 10 * KERNEL_THREAD_STEP)
    {
        struct thread *current_thread;

        cli();

        current_thread = thread_current();
        current_thread->wake_millisecond = tick_to_millisecond() + millisecond;
        ++(current_thread->ref);

        /* 暂停当前线程，会自动切换出去 */
        thread_suspend(current_thread->tid);
    }
    else
    {
        delay(millisecond);
    }
}

void sleeper_polling()
{
    for (;;)
    {
        struct thread *next;
        struct thread *node = thread_list(THREAD_SUSPEND);
        uint32_t min_wake_millisecond = ~0;

        while (node != NULL)
        {
            next = node->next;
            if (node->wake_millisecond <= tick_to_millisecond())
            {
                cli();

                --(node->ref);

                if (node->status != THREAD_DIED)
                {
                    /* node唤醒后会切换为线程运行队列的第一个线程 */
                    thread_wake(node->tid);
                }

                sti();
            }
            else if (node->wake_millisecond < min_wake_millisecond)
            {
                min_wake_millisecond = node->wake_millisecond;
            }
            node = next;
        }
        /* 如果所有睡眠的线程都将较晚才唤醒，则让出cpu使用权，否则继续轮询 */
        if (min_wake_millisecond > 10)
        {
            thread_yield();
        }
    }
}

void print_tick()
{
    uint32_t current_tick = tick_to_millisecond();
    uint32_t second = current_tick / 1000;
    uint32_t msecond = current_tick - second * 1000;
    size_t tick_str_len = 12;
    char string[34];

    tick_str_len -= base10_string(second, 0, string);
    tick_str_len -= base10_string(msecond, 0, string);

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
