#include <bitmap.h>
#include <console.h>
#include <device.h>
#include <eval.h>
#include <malloc.h>
#include <memory.h>
#include <interrupt.h>
#include <kernel.h>
#include <rtc.h>
#include <thread.h>
#include <timer.h>
#include <vbe.h>

void __attribute__((noreturn)) entry(void)
{
    init_vbe();
    init_console(get_screen_width(), get_screen_height());
    init_8259a();
    init_idt();
    init_keyboard();
    init_timer();
    init_memory();
    init_bitmap(get_total_memory_bytes());
    init_rtc();

    start_thread(&eval);
}
