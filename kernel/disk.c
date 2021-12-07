#include <disk.h>
#include <device.h>
#include <io.h>
#include <kernel.h>
#include <malloc.h>
#include <string.h>

#include "fs.h"

static struct disk *disk_list = NULL;

static struct disk *find_disk_by_id(uint32_t id)
{
    struct disk *node = disk_list;
    /* 传入的id可能高16位不是0，要手动清理 */
    id = (uint16_t)id;

    while (node != NULL)
    {
        if (node->id == id)
        {
            return node;
        }
        node = node->next;
    }

    return NULL;
}

struct disk *disk_register(char *name)
{
    static union
    {
        uint32_t all;
        struct
        {
            uint8_t number;
            uint8_t flag;
            uint8_t zero;
            uint8_t unused;
        };
    } disk_id = { .all = KERNEL_DISK_ID_START };
    struct disk *new_disk;

    if (disk_id.all > KERNEL_DISK_ID_END)
    {
        return NULL;
    }

    new_disk = (struct disk *)malloc(sizeof(struct disk));

    new_disk->id = disk_id.all;
    new_disk->name = name;
    new_disk->total = 0;
    new_disk->free = 0;
    new_disk->used = 0;
    new_disk->ref = 0;
    new_disk->device_read = NULL;
    new_disk->device_write = NULL;
    new_disk->fs_file_read = NULL;
    new_disk->fs_file_write = NULL;
    new_disk->fs_file_open = NULL;
    new_disk->fs_file_close = NULL;
    new_disk->device = NULL;
    new_disk->fs = NULL;
    new_disk->fs_type = NULL;
    new_disk->fs_filesize = 0;
    new_disk->next = disk_list;

    disk_list = new_disk;
    ++(disk_id.number);

    return new_disk;
}

void disk_format(uint32_t id, char *fs_type)
{
    int i;
    struct disk *disk;

    cli();

    disk = find_disk_by_id(*(unsigned int *)id);

    sti();

    if (disk == NULL)
    {
        printk("invalid disk `%s'\n", id);
        return;
    }
    if (disk->ref != 0)
    {
        printk("disk `%s' is occupied by another execution flow\n", id);
        return;
    }

    for (i = 0; i < ARRAY_SIZE(fs_opts); ++i)
    {
        if (!strcmp(fs_type, fs_opts[i].type))
        {
            printk("disk `%s' format to `%s' %s\n", (char *)id, fs_type, fs_opts[i].format(disk) ? "fail" : "ok");
            return;
        }
    }

    printk("%s `%s'\n", UNKNOWN_FS_TYPE, fs_type);
}

void init_disk()
{
    struct disk *node = disk_list;
    size_t disk_number = 0;
    int i;

    while (node != NULL)
    {
        for (i = 0; i < ARRAY_SIZE(fs_opts); ++i)
        {
            if (!fs_opts[i].check(node))
            {
                break;
            }
        }
        ++disk_number;
        node = node->next;
    }

    if (disk_number > 0)
    {
        uint32_t disk_id_start = KERNEL_DISK_ID_START;
        uint32_t disk_id_end = KERNEL_DISK_ID_START + (disk_number - 1);
        LOG("found %d disks[%s ~ %s]\n", disk_number, &disk_id_start, &disk_id_end);
    }
    else
    {
        LOG("found 0 disks\n");
    }
}

void print_disk(uint32_t disk_id)
{
    struct disk *node = disk_list;
    int len;

    if (disk_id != 0)
    {
        struct disk *node;

        if (disk_id >= KERNEL_DISK_ID_START && disk_id <= KERNEL_DISK_ID_END)
        {
            printk("disk id `%s' is invalid\n", (char *)disk_id);
            return;
        }

        if ((node = find_disk_by_id(*(unsigned int *)disk_id)) != NULL)
        {
            printk("id:\t\t\t %s\n", (char *)&node->id);
            printk("name:\t\t %s\n", node->name);
            printk("fs type:\t %s\n", node->fs_type == NULL ? UNKNOWN_FS_TYPE : node->fs_type);
            printk("total:\t\t %d bytes\n", node->total);
            printk("free:\t\t %d bytes\n", node->free);
            printk("used:\t\t %d bytes\n", node->used);
            printk("occupied:\t %d\n", node->ref);
        }
        else
        {
            printk("can't find disk `%s'\n", disk_id);
        }
    }
    else
    {
        set_color_invert();
        printk(" disk id | total(MB) | free(MB) | used(MB) ");
        set_color_invert();
        printk("\n");

        while (node != NULL)
        {
            printk(" %s\t\t |", &node->id);
            len = printk(" %d", node->total / MB);
            while (len < 11)
            {
                printk(" ");
                ++len;
            }
            len = printk("| %d", node->free / MB);
            while (len < 11)
            {
                printk(" ");
                ++len;
            }
            len = printk("| %d", node->used / MB);
            while (len < 11)
            {
                printk(" ");
                ++len;
            }
            printk("\n");
            node = node->next;
        }
    }
}
