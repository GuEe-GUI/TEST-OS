#include <bitmap.h>
#include <console.h>
#include <device.h>
#include <disk.h>
#include <eval.h>
#include <interrupt.h>
#include <kernel.h>
#include <malloc.h>
#include <rtc.h>
#include <string.h>
#include <thread.h>
#include <timer.h>

EVAL_VOID(cls, "Clear console")(int argc, char**argv)
{
    console_cls();
}

EVAL_VOID(powerdown, "Machine power down")(int argc, char**argv)
{
    cli();

    LOG("machine power down");

    /* QEMU (newer) */
    __asm__ volatile ("outw %%ax, %%dx"::"d"(0x604), "a"(0x2000));
    /* QEMU (2.0)， bochs */
    __asm__ volatile ("outw %%ax, %%dx"::"d"(0xb004), "a"(0x2000));

    PANIC("powerdown fail\n");
}

EVAL_VOID(rtc, "Get rtc time")(int argc, char**argv)
{
    struct rtc_time *time = read_rtc();

    printk("%d/%d/%d %d:%d:%d\n", time->year, time->month, time->day, time->hour, time->minute, time->second);
}

EVAL_VOID(thread, "Thread control")(int argc, char**argv)
{
    tid_t tid = 0;
    int i = 0;

    if (argc < 2 || argv[1][0] != '-' || argv[1][1] == '\0' || strlen(argv[1]) > 2)
    {
        printk("Usage: thread [options] [tid]\nOptions:\n");
        printk("  -w\tWake a thread\n");
        printk("  -s\tSuspend a thread\n");
        printk("  -e\tExit a thread\n");
        printk("  -l\tList all threads info (without tid)\n");
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

EVAL_VOID(interrupt, "Interrupt control")(int argc, char**argv)
{
    uint8_t vector = 0;
    int i = 0;

    if (argc < 2 || argv[1][0] != '-' || argv[1][1] == '\0' || strlen(argv[1]) > 2)
    {
        printk("Usage: interrupt [options] [vector]\nOptions:\n");
        printk("  -e\tEnable an interrupt\n");
        printk("  -d\tDisable an interrupt\n");
        printk("  -l\tList all interrupts info (without vector)\n");
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
        interrupt_enable(vector);
        break;
    }
    case 'd':
    {
        interrupt_disable(vector);
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

EVAL_VOID(mem, "Memory info")(int argc, char**argv)
{
    print_mem();
}

EVAL_VOID(disk, "Disk control")(int argc, char**argv)
{
    if (argc < 2 || argv[1][0] != '-' || argv[1][1] == '\0' || strlen(argv[1]) > 2)
    {
        printk("Usage: disk [options] [disk id]\nOptions:\n");
        printk("  -f [type]\tFormat a disk with file system of type\n");
        printk("  -d\t\tGet details of disk\n");
        printk("  -l\t\tList all disks info (without disk id)\n");
        return;
    }

    if (argv[1][1] == 'l')
    {
        print_disk(0);
        return;
    }
    else if (argc < 3)
    {
no_disk_name:
        printk("no input disk id\n");
        return;
    }

    switch (argv[1][1])
    {
    case 'f':
    {
        if (argc < 4)
        {
            goto no_disk_name;
        }
        disk_format((uint32_t)argv[3], argv[2]);
        break;
    }
    case 'd':
    {
        print_disk((uint32_t)argv[2]);
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

EVAL_VOID(md, "Make a directory")(int argc, char**argv)
{
}

EVAL_VOID(dir, "Display files in this directory")(int argc, char**argv)
{
}

EVAL_VOID(rename, "Rename a file/directory")(int argc, char**argv)
{
}

EVAL_VOID(cd, "Change disk/directory")(int argc, char**argv)
{
}

EVAL_VOID(copy, "Copy a file/directory")(int argc, char**argv)
{
}

EVAL_VOID(del, "Delete a file/directory")(int argc, char**argv)
{
    if (argc < 2)
    {
        printk("no input file\n");
        return;
    }
}

EVAL_VOID(pushf, "Push data to file")(int argc, char**argv)
{
    if (argc < 2 || argv[1][0] != '>' || argv[1][1] == '\0' || strlen(argv[1]) > 2)
    {
        printk("Usage: pushf [data] [options] [file]\nOptions:\n");
        printk("  > \tOverlay data to file\n");
        printk("  >>\tAppend data to file\n");
        return;
    }
}

EVAL_VOID(popf, "Pop data from file")(int argc, char**argv)
{
    if (argc < 2 || argv[1][0] != '-' || argv[1][1] == '\0' || strlen(argv[1]) > 2)
    {
        printk("Usage: popf [options] [file]\nOptions:\n");
        printk("  -a\tPrint with ascii\n");
        printk("  -h\tPrint with hex\n");
        return;
    }
}
