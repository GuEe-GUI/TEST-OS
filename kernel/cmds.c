#include <bitmap.h>
#include <console.h>
#include <device.h>
#include <eval.h>
#include <kernel.h>
#include <rtc.h>
#include <string.h>
#include <thread.h>
#include <timer.h>

EVAL_VOID(cls, "clear console")(int argc, char**argv)
{
    console_cls();
}

EVAL_VOID(date, "get rtc date")(int argc, char**argv)
{
    struct rtc_time *time = read_rtc();

    printk("%d/%d/%d %d:%d:%d\n", time->year, time->month, time->day, time->hour, time->minute, time->second);
}

EVAL_VOID(thread, "thread control")(int argc, char**argv)
{
    tid_t tid = 0;
    int i;

    if (argc < 2 || argv[1][0] != '-' || argv[1][1] == '\0' || strlen(argv[1]) > 2)
    {
        printk("usage: thread [options] [tid]\noptions:\n");
        printk("  -w\twake a thread\n");
        printk("  -s\tsuspend a thread\n");
        printk("  -e\texit a thread\n");
        printk("  -l\tlist all threads info\n");
        return;
    }

    if (argv[1][1] == 'l')
    {
        print_thread();
        return;
    }
    else if (argc < 3)
    {
        printk("no input tid\n");
        return;
    }

    while (argv[2][i] != '\0')
    {
        if (argv[2][i] < '0' || argv[2][i] > '9')
        {
            printk("unrecognized tid `%s'\n", argv[2]);
            return;
        }
        tid *= 10;
        tid += argv[2][i++] - '0';
    }

    switch (argv[1][1])
    {
    case 'w':
    {
        thread_wake(tid);
        break;
    }
    case 's':
    {
        thread_suspend(tid);
        break;
    }
    case 'e':
    {
        thread_wait(tid);
        break;
    }
    default:
    {
        printk("unrecognized option `%s'\n", &argv[1][1]);
        break;
    }
    }

    return;
}

EVAL_VOID(mem, "memory info")(int argc, char**argv)
{
    print_mem();
}

EVAL_VOID(poweroff, "poweroff...")(int argc, char**argv)
{
    cli();

    LOG("poweroff...");

    /* QEMU (newer) */
    __asm__ volatile ("outw %%ax, %%dx"::"d"(0x604), "a"(0x2000));
    /* QEMU (2.0)， bochs */
    __asm__ volatile ("outw %%ax, %%dx"::"d"(0xb004), "a"(0x2000));

    PANIC("poweroff fail\n");
}
