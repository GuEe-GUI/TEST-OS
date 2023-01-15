#include <bitmap.h>
#include <console.h>
#include <device.h>
#include <disk.h>
#include <eval.h>
#include <interrupt.h>
#include <io.h>
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

EVAL_VOID(version, "Dump OS version")(int argc, char**argv)
{
    console_dump_version();
}

EVAL_VOID(powerdown, "Machine power down")(int argc, char**argv)
{
    cli();

    LOG("machine power down");

    /* QEMU (newer) */
    __asm__ volatile ("outw %%ax, %%dx"::"d"(0x604), "a"(0x2000));
    /* QEMU (2.0)，bochs */
    __asm__ volatile ("outw %%ax, %%dx"::"d"(0xb004), "a"(0x2000));

    PANIC("powerdown fail");
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
    struct dir dir;

    if (argc < 2)
    {
        printk("no input directory name\n");
        return;
    }

    if (disk_dir_open(get_eval_path(), &dir) < 0)
    {
        goto fail;
    }

    if (!disk_dir_create(&dir, argv[3], NULL))
    {
        goto success;
    }

fail:
    printk("make `%s' directory fail\n", argv[1]);
success:
    disk_dir_close(&dir);
}

EVAL_VOID(dir, "Display files and directories in this directory")(int argc, char**argv)
{
    uint32_t disk_id;
    size_t len, size;
    struct dir dir;
    struct dir_entry dir_entry;

    if (disk_dir_open(get_eval_path(), &dir))
    {
        return;
    }

    disk_id = DISK_PATH_ID(get_eval_path());

    memset(&dir_entry, 0, sizeof(dir_entry));

    while (disk_dir_read(&dir, &dir_entry) >= 0)
    {
        len = printk("%s ", dir_entry.name);
        put_space(len, 16);
        if (dir_entry.attribute == DIR_ENTRY_DIR)
        {
            set_color_invert();
            printk("(DIR)");
            set_color_invert();
            printk("\n");
        }
        else
        {
            size = 0;
            disk_fs_request(disk_id, FS_DIR_ENTRY_SIZE, &dir_entry, &size);
            printk("%d bytes\n", size);
        }
    }

    disk_dir_close(&dir);
}

EVAL_VOID(rename, "Rename a file/directory")(int argc, char**argv)
{
    uint32_t disk_id;

    if (argc < 3)
    {
        printk("Usage: rename [name1] [name2]\n");
        return;
    }

    disk_id = DISK_PATH_ID(get_eval_path());

    if (disk_fs_request(disk_id, FS_FILE_RENAME, PTR_LIST(argv[1], argv[2]), NULL) &&
        disk_fs_request(disk_id, FS_DIR_RENAME, PTR_LIST(argv[1], argv[2]), NULL))
    {
        printk("rename `%s' to `%s' fail\n", argv[1], argv[2]);
    }
}

EVAL_VOID(cd, "Change disk/directory")(int argc, char**argv)
{
    char path[KERNEL_DISK_MAX_PATH];

    if (argc < 2)
    {
        printk("Usage: cd [path]\n");
        return;
    }

    strcpy(path, get_eval_path());

    if (argv[1][KERNEL_DISK_ID_LEN - 1] == ':')
    {
        uint32_t target_disk_id = DISK_PATH_ID(argv[1]);
        uint32_t current_disk_id = DISK_PATH_ID(path);

        if (argv[1][KERNEL_DISK_ID_LEN] == '\0')
        {
            if (target_disk_id != current_disk_id)
            {
                strncpy(path, argv[1], KERNEL_DISK_ID_LEN);
                path[KERNEL_DISK_ID_LEN] = '\0';

                goto set_path;
            }
            return;
        }

        printk("invalid disk id `%s'\n", argv[1]);
        return;
    }

    if (!strcmp(argv[1], ".."))
    {
        int i = 0;

        while (path[i] != '\0')
        {
            ++i;
        }
        while (i >= 0 && path[i] != '/')
        {
            --i;
        }
        --i;
        while (i >= 0 && path[i] != '/')
        {
            --i;
        }
        path[i + 1] = '\0';
    }
    else
    {
        strcpy(&path[strlen(path)], argv[1]);
    }

set_path:
    if (set_eval_path(path))
    {
        printk("entry %s fail\n", argv[1]);
    }
}

EVAL_VOID(copy, "Copy a file/directory")(int argc, char**argv)
{
    uint32_t disk_id;
    char *path;
    size_t size, path_len;
    struct file file1, file2;
    struct dir dir;
    int flag = 0;

    if (argc < 3)
    {
        printk("Usage: copy [name1] [name2]\n");
        return;
    }

    if (!strcmp(argv[1], argv[2]))
    {
        printk("must be a different name");
        return;
    }

    path = get_eval_path();
    disk_id = *(uint32_t *)path;
    path_len = strlen(path);

    strcpy(&file1.path[path_len], argv[1]);
    strcpy(&file2.path[path_len], argv[2]);

    if (disk_file_open(file1.path, &file1))
    {
        printk("open file `%s'");
    }

    if (disk_dir_open(path, &dir) < 0)
    {
        goto fail;
    }

    if (!disk_dir_create_entry(&dir, argv[2], 0, NULL))
    {
        goto success;
    }

fail:
    flag = printk("create file `%s' fail\n", argv[2]);
success:
    disk_dir_close(&dir);

    if (flag)
    {
        return;
    }

    disk_fs_request(disk_id, FS_FILE_SIZE, &file1, &size);

    if (size > 0)
    {
        char *buffer;

        buffer = (char *)malloc(size);
        ASSERT(!disk_file_open(file2.path, &file2));

        disk_file_read(&file1, buffer, 0, size);
        disk_file_write(&file2, buffer, 0, size);

        disk_file_close(&file2);
        free(buffer);
    }

    disk_file_close(&file1);
}

EVAL_VOID(del, "Delete a file/directory")(int argc, char**argv)
{
    uint32_t disk_id;

    if (argc < 2)
    {
        printk("no input file\n");
        return;
    }

    disk_id = DISK_PATH_ID(get_eval_path());

    if (disk_fs_request(disk_id, FS_FILE_DELETE, argv[1], NULL) &&
        disk_fs_request(disk_id, FS_DIR_DELETE, argv[1], NULL))
    {
        printk("delete `%s' fail\n", argv[1]);
    }
}

EVAL_VOID(pushf, "Push data to file")(int argc, char**argv)
{
    char path[KERNEL_DISK_MAX_PATH];
    int length;
    off_t offset;
    struct file file;

    if (argc < 4 || argv[2][0] != '>' || strlen(argv[2]) > 2)
    {
        printk("Usage: pushf [data] [options] [file]\nOptions:\n");
        printk("  > \tOverlay data to file\n");
        printk("  >>\tAppend data to file\n");
        return;
    }

    if (argv[2][2] != '\0')
    {
        offset = 0;
    }
    else if (argv[2][2] == '>')
    {
        offset = !0;
    }
    else
    {
        printk("unrecognized option `%s'\n", &argv[2][2]);
        return;
    }

    strcpy(path, get_eval_path());
    length = strlen(path);
    strncpy(&path[length], argv[3], KERNEL_DISK_MAX_PATH);

    if (disk_file_open(path, &file) < 0)
    {
        struct dir dir;
        int flag = 0;

        if (disk_dir_open(get_eval_path(), &dir) < 0)
        {
            goto fail;
        }

        if (!disk_dir_create_entry(&dir, argv[3], 0, NULL))
        {
            goto success;
        }

    fail:
        flag = printk("create file `%s' fail\n", argv[3]);
    success:
        disk_dir_close(&dir);

        if (flag)
        {
            return;
        }
    }

    if (offset)
    {
        if (disk_fs_request(DISK_PATH_ID(get_eval_path()), FS_FILE_SIZE, &file, &offset))
        {
            offset = 0;
        }
    }

    disk_file_write(&file, argv[1], offset, strlen(argv[1]));
    disk_file_close(&file);
}

EVAL_VOID(popf, "Pop data from file")(int argc, char**argv)
{
    int i;
    int length;
    int cols = get_console_cols() / 3 - 1,  col = 0;
    off_t offset;
    size_t out_size, in_size;
    struct file file;
    char *path = get_eval_path();
    char buffer[512];

    if (argc < 3 || argv[1][0] != '-' || argv[1][1] == '\0' || strlen(argv[1]) > 2)
    {
        printk("Usage: popf [options] [file]\nOptions:\n");
        printk("  -a\tPrint with ascii\n");
        printk("  -h\tPrint with hex\n");
        return;
    }

    length = strlen(path);
    strcpy(buffer, path);
    strcpy(&buffer[length], argv[2]);

    if (disk_file_open(buffer, &file) < 0)
    {
        printk("open file `%s' fail\n", argv[2]);
        return;
    }

    for (offset = 0, in_size = sizeof(buffer);;)
    {
        out_size = disk_file_read(&file, buffer, offset, in_size);

        if (out_size == 0)
        {
            if (in_size != sizeof(buffer))
            {
                break;
            }
            in_size = 1;
            if (offset != 0)
            {
                offset -= sizeof(buffer);
                memset(buffer, 0, sizeof(buffer));
            }
            continue;
        }
        else
        {
            if (argv[1][1] == 'a')
            {
                for (i = 0; i < out_size; ++i)
                {
                    printk("%c", buffer[i]);
                }
            }
            else if (argv[1][1] == 'h')
            {
                for (i = 0; i < out_size; ++i, ++col)
                {
                    if (buffer[i] < 0x10)
                    {
                        printk("0");
                    }
                    printk("%x ", buffer[i]);

                    if (col >= cols)
                    {
                        printk("\n");
                        col = 0;
                    }
                }
            }
            else
            {
                printk("unrecognized option `%s'\n", &argv[1][1]);
                break;
            }
        }

        offset += in_size;
    }

    if (col)
    {
        printk("\n");
    }

    disk_file_close(&file);
}

EVAL_VOID(graphic, "Graphic test")(int argc, char**argv)
{
    int i;
    uint32_t width = get_screen_width();
    uint32_t height = get_screen_height();
    uint32_t x, y;
    uint32_t colors[] =
    {
        RGB(0, 0, 0),
        RGB(255, 0, 0),
        RGB(0, 255, 0),
        RGB(0, 0, 255),
        RGB(255, 255, 255),
    };

    for (i = 0; i < ARRAY_SIZE(colors); ++i)
    {
        uint8_t r = RED(colors[i]);
        uint8_t g = GREEN(colors[i]);
        uint8_t b = BLUE(colors[i]);

        for (x = 0; x < width; ++x)
        {
            for (y = 0; y < height; ++y)
            {
                put_pixel(x, y, r, g, b);
            }
        }

        delay(500);
    }

    for (x = 0; x < width; ++x)
    {
        for (y = 0; y < height; ++y)
        {
            uint8_t r = soft_rand() & 0xff;
            uint8_t g = soft_rand() & 0xff;
            uint8_t b = soft_rand() & 0xff;
            put_pixel(x, y, r, g, b);
        }
    }

    return;
}
