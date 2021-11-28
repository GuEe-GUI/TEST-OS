#include <disk.h>
#include <device.h>
#include <io.h>
#include <kernel.h>
#include <malloc.h>
#include <string.h>

#include <fat32.h>

static struct
{
    char *type;
    int (*check)(struct disk *disk);
    int (*format)(struct disk *disk);
} fs_opts[] =
{
    { .type = "fat32", .check = &fat32_check, .format = &fat32_format },
};

static struct disk *disk_list = NULL;

static struct disk *find_disk_by_id(char id)
{
    struct disk *node = disk_list;

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

void disk_register(char *name, int (*read)(), int (*write)(), void *device)
{
    static char disk_id = KERNEL_DISK_ID_START;
    struct disk *new_disk;
    int i;

    if (disk_id > KERNEL_DISK_ID_END)
    {
        return;
    }

    new_disk = (struct disk *)malloc(sizeof(struct disk));

    new_disk->id = disk_id++;
    new_disk->name = name;
    new_disk->fs_type = NULL;
    new_disk->total = 0;
    new_disk->free = 0;
    new_disk->used = 0;
    new_disk->ref = 0;
    new_disk->read = read;
    new_disk->write = write;
    new_disk->device = device;
    new_disk->next = disk_list;

    for (i = 0; i < ARRAY_SIZE(fs_opts); ++i)
    {
        if (!fs_opts[i].check(new_disk))
        {
            disk_list = new_disk;
            return;
        }
    }

    --disk_id;
    free(new_disk);
}

void disk_format(char id, char *fs_type)
{
    int i;
    struct disk *disk;

    cli();

    disk = find_disk_by_id(id);

    sti();

    if (disk == NULL)
    {
        printk("invalid disk `%c'\n", id);
        return;
    }
    if (disk->ref != 0)
    {
        printk("disk `%c' is occupied by another execution flow\n", id);
        return;
    }

    for (i = 0; i < ARRAY_SIZE(fs_opts); ++i)
    {
        if (!strcmp(fs_type, fs_opts[i].type))
        {
            fs_opts[i].format(disk);
            return;
        }
    }

    printk("unknown file system `%s'\n", fs_type);
}

void init_disk()
{
    struct disk *node = disk_list;
    size_t disk_number = 0;

    while (node != NULL)
    {
        ++disk_number;
        node = node->next;
    }

    LOG("found %d disks [%c ~ %c]\n", disk_number, KERNEL_DISK_ID_START, (disk_number - 1) + KERNEL_DISK_ID_START);
}

void print_disk(int disk_id)
{
    struct disk *node = disk_list;
    int len;

    if (disk_id >= KERNEL_DISK_ID_START)
    {
        printk("disk id = %d", disk_id);
    }
    else
    {
        set_color_invert();
        printk(" disk id | total(MB) | free(MB) | used(MB) ");
        set_color_invert();
        printk("\n");

        while (node != NULL)
        {
            printk(" %c\t\t |", node->id);
            len = printk(" %d", node->total);
            while (len < 11)
            {
                printk(" ");
                ++len;
            }
            len = printk("| %d", node->free);
            while (len < 11)
            {
                printk(" ");
                ++len;
            }
            len = printk("| %d", node->used);
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
