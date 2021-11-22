#include <bitmap.h>
#include <console.h>
#include <device.h>
#include <eval.h>
#include <interrupt.h>
#include <kernel.h>
#include <malloc.h>
#include <rtc.h>
#include <string.h>
#include <thread.h>
#include <timer.h>

EVAL_VOID(cls, "clear console")(int argc, char**argv)
{
    console_cls();
}

EVAL_VOID(rtc, "get rtc time")(int argc, char**argv)
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
        thread_exit(tid);
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

EVAL_VOID(interrupt, "interrupt control")(int argc, char**argv)
{
    uint8_t vector = 0;
    int i;

    if (argc < 2 || argv[1][0] != '-' || argv[1][1] == '\0' || strlen(argv[1]) > 2)
    {
        printk("usage: interrupt [options] [vector]\noptions:\n");
        printk("  -e\tenable an interrupt\n");
        printk("  -d\tdisable an interrupt\n");
        printk("  -l\tlist all interrupts info\n");
        return;
    }

    if (argv[1][1] == 'l')
    {
        print_interrupt();
        return;
    }
    else if (argc < 3)
    {
        printk("no input vector\n");
        return;
    }

    while (argv[2][i] != '\0')
    {
        if (argv[2][i] < '0' || argv[2][i] > '9')
        {
            printk("unrecognized vector `%s'\n", argv[2]);
            return;
        }
        vector *= 10;
        vector += argv[2][i++] - '0';
    }

    switch (argv[1][1])
    {
    case 'e':
    {
        enable_interrupt(vector);
        break;
    }
    case 'd':
    {
        disable_interrupt(vector);
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

EVAL_VOID(powerdown, "powerdown...")(int argc, char**argv)
{
    cli();

    LOG("powerdown...");

    /* QEMU (newer) */
    __asm__ volatile ("outw %%ax, %%dx"::"d"(0x604), "a"(0x2000));
    /* QEMU (2.0)， bochs */
    __asm__ volatile ("outw %%ax, %%dx"::"d"(0xb004), "a"(0x2000));

    PANIC("powerdown fail\n");
}

EVAL_VOID(dir, "display files in this directory")(int argc, char**argv)
{
}

EVAL_VOID(rename, "rename a file/directory")(int argc, char**argv)
{
}

EVAL_VOID(copy, "copy a file/directory")(int argc, char**argv)
{
}

EVAL_VOID(del, "del a file/directory")(int argc, char**argv)
{
    if (argc < 2)
    {
        printk("no input file\n");
        return;
    }
}

EVAL_VOID(log, "write/read file")(int argc, char**argv)
{
}
