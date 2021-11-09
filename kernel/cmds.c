#include <console.h>
#include <device.h>
#include <eval.h>
#include <kernel.h>
#include <rtc.h>

EVAL_VIOD(cls, "clear console")(int argc, char**argv)
{
    console_cls();
}

EVAL_VIOD(date, "get rtc date")(int argc, char**argv)
{
    struct rtc_time *time = read_rtc();

    printk("%d/%d/%d %d:%d:%d\n", time->year, time->month, time->day, time->hour, time->minute, time->second);
}

EVAL_VIOD(poweroff, "poweroff...")(int argc, char**argv)
{
    cli();

    LOG("poweroff...");

    /* QEMU (newer) */
    __asm__ volatile ("outw %%ax, %%dx"::"d"(0x604), "a"(0x2000));
    /* QEMU (2.0)， bochs */
    __asm__ volatile ("outw %%ax, %%dx"::"d"(0xb004), "a"(0x2000));

    PANIC("poweroff fail\n");
}
