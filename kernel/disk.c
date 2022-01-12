#include <disk.h>
#include <device.h>
#include <io.h>
#include <kernel.h>
#include <malloc.h>
#include <string.h>

#include "fs.h"

static struct disk *disk_list = NULL;
static pe_lock_t disk_lock = 0;

static struct disk *find_disk_by_id(uint32_t id)
{
    struct disk *node, *ret = NULL;

    pe_lock(&disk_lock);

    node = disk_list;
    /* 传入的id可能高16位不是0，要手动清理 */
    id = (uint16_t)id;

    while (node != NULL)
    {
        if (node->id == id)
        {
            ret = node;
            break;
        }
        node = node->next;
    }

    pe_unlock(&disk_lock);

    return ret;
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

    ASSERT(new_disk != NULL);

    new_disk->id = disk_id.all;
    new_disk->name = name;
    new_disk->total = 0;
    new_disk->free = 0;
    new_disk->used = 0;
    new_disk->ref = 0;
    new_disk->device_read = NULL;
    new_disk->device_write = NULL;
    new_disk->fs_file_open = NULL;
    new_disk->fs_file_close = NULL;
    new_disk->fs_file_read = NULL;
    new_disk->fs_file_write = NULL;
    new_disk->fs_file_seek = NULL;
    new_disk->fs_dir_open = NULL;
    new_disk->fs_dir_close = NULL;
    new_disk->fs_dir_read = NULL;
    new_disk->fs_dir_create_entry = NULL;
    new_disk->fs_dir_create = NULL;
    new_disk->fs_request = NULL;
    new_disk->device = NULL;
    new_disk->fs = NULL;
    new_disk->fs_type = UNKNOWN_FS_TYPE;
    new_disk->next = disk_list;

    disk_list = new_disk;
    ++(disk_id.number);

    return new_disk;
}

void disk_format(uint32_t id, char *fs_type)
{
    int i;
    struct disk *disk;

    disk = find_disk_by_id(*(unsigned int *)id);

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
            int fs_ret = fs_opts[i].format(disk);

            printk("disk `%s' format to `%s' %s\n", (char *)id, fs_type, fs_ret ? "fail" : "ok");

            if (!fs_ret)
            {
                fs_ret = fs_opts[i].check(disk);

                printk("disk `%s' check %s\n", (char *)id, fs_ret ? "fail" : "ok");
            }

            return;
        }
    }

    printk("%s `%s'\n", UNKNOWN_FS_TYPE, fs_type);
}

void init_disk()
{
    struct disk *node = disk_list;
    uint64_t disk_number = 0;
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
        i = KERNEL_DISK_ID_START + (uint16_t)(disk_number - 1);
        disk_number <<= 32;
        disk_number |= KERNEL_DISK_ID_START;
        LOG("found %d disks start = \"%s\", end = \"%s\"", (uint32_t)(disk_number >> 32), &disk_number, &i);
    }
    else
    {
        LOG("found 0 disks");
    }
}

int disk_file_open(const char *path, struct file *file)
{
    if (path != NULL && file != NULL)
    {
        uint32_t disk_id = *(uint16_t *)path;

        if (disk_id >= KERNEL_DISK_ID_START && disk_id <= KERNEL_DISK_ID_END)
        {
            struct disk *disk = find_disk_by_id(disk_id);

            if (disk != NULL && disk->fs_file_open != NULL)
            {
                int fs_ret;

                file->disk = disk_id;

                while (*path && *path != '/')
                {
                    ++path;
                }

                ++(disk->ref);
                fs_ret = disk->fs_file_open(disk, path, file);
                --(disk->ref);

                return fs_ret;
            }
        }
    }

    return -1;
}

int disk_file_close(struct file* file)
{
    if (file != NULL)
    {
        if (file->disk >= KERNEL_DISK_ID_START && file->disk <= KERNEL_DISK_ID_END)
        {
            struct disk *disk = find_disk_by_id(file->disk);

            if (disk != NULL && disk->fs_file_close != NULL)
            {
                int fs_ret;

                ++(disk->ref);
                fs_ret = disk->fs_file_close(disk, file);
                --(disk->ref);

                if (file->fs_file != NULL)
                {
                    free(file->fs_file);
                }

                return fs_ret;
            }
        }
    }

    return -1;
}

size_t disk_file_read(struct file *file, void *buffer, off_t offset, size_t length)
{
    if (file != NULL && buffer != NULL && length != 0)
    {
        if (file->disk >= KERNEL_DISK_ID_START && file->disk <= KERNEL_DISK_ID_END)
        {
            struct disk *disk = find_disk_by_id(file->disk);

            if (disk != NULL && disk->fs_file_read != NULL)
            {
                int fs_ret;

                ++(disk->ref);
                fs_ret = disk->fs_file_read(disk, file, buffer, offset, length);
                --(disk->ref);

                return fs_ret;
            }
        }
    }

    return 0;
}

size_t disk_file_write(struct file *file, const void *buffer, off_t offset, size_t length)
{
    if (file != NULL && buffer != NULL && length != 0)
    {
        if (file->disk >= KERNEL_DISK_ID_START && file->disk <= KERNEL_DISK_ID_END)
        {
            struct disk *disk = find_disk_by_id(file->disk);

            if (disk != NULL && disk->fs_file_write != NULL)
            {
                int fs_ret;

                ++(disk->ref);
                fs_ret = disk->fs_file_write(disk, file, buffer, offset, length);
                --(disk->ref);

                return fs_ret;
            }
        }
    }

    return 0;
}

int disk_file_seek(struct file *file, off_t offset, int whence)
{
    if (file != NULL && (whence >= FILE_SEEK_SET && whence <= FILE_SEEK_END))
    {
        if (file->disk >= KERNEL_DISK_ID_START && file->disk <= KERNEL_DISK_ID_END)
        {
            struct disk *disk = find_disk_by_id(file->disk);

            if (disk != NULL && disk->fs_file_seek != NULL)
            {
                int fs_ret;

                ++(disk->ref);
                fs_ret = disk->fs_file_seek(disk, file, offset, whence);
                --(disk->ref);

                return fs_ret;
            }
        }
    }

    return -1;
}

int disk_dir_open(const char *path, struct dir *dir)
{
    if (path != NULL && dir != NULL)
    {
        uint32_t disk_id = *(uint16_t *)path;

        if (disk_id >= KERNEL_DISK_ID_START && disk_id <= KERNEL_DISK_ID_END)
        {
            struct disk *disk = find_disk_by_id(disk_id);

            if (disk != NULL && disk->fs_dir_open != NULL)
            {
                int fs_ret;

                dir->disk = disk_id;

                while (*path && *path != '/')
                {
                    ++path;
                }

                ++(disk->ref);
                fs_ret = disk->fs_dir_open(disk, path, dir);
                --(disk->ref);

                return fs_ret;
            }
        }
    }

    return -1;
}

int disk_dir_close(struct dir *dir)
{
    if (dir != NULL)
    {
        if (dir->disk >= KERNEL_DISK_ID_START && dir->disk <= KERNEL_DISK_ID_END)
        {
            struct disk *disk = find_disk_by_id(dir->disk);

            if (disk != NULL && disk->fs_dir_close != NULL)
            {
                int fs_ret;

                ++(disk->ref);
                fs_ret = disk->fs_dir_close(disk, dir);
                --(disk->ref);

                if (dir->fs_dir != NULL)
                {
                    free(dir->fs_dir);
                }

                return fs_ret;
            }
        }
    }

    return -1;
}

int disk_dir_read(struct dir *dir, struct dir_entry *entry)
{
    if (dir != NULL && entry != NULL)
    {
        if (dir->disk >= KERNEL_DISK_ID_START && dir->disk <= KERNEL_DISK_ID_END)
        {
            struct disk *disk = find_disk_by_id(dir->disk);

            if (disk != NULL && disk->fs_dir_read != NULL)
            {
                int fs_ret;

                ++(disk->ref);
                fs_ret = disk->fs_dir_read(disk, dir, entry);
                --(disk->ref);

                return fs_ret;
            }
        }
    }

    return -1;
}

int disk_dir_create_entry(struct dir *dir, const char *name, enum DIR_ENTRY_ATTRIB attribute, struct dir_entry *dir_entry)
{
    if (dir != NULL && name != NULL)
    {
        if (dir->disk >= KERNEL_DISK_ID_START && dir->disk <= KERNEL_DISK_ID_END)
        {
            struct disk *disk = find_disk_by_id(dir->disk);

            if (disk != NULL && disk->fs_dir_create_entry != NULL)
            {
                int fs_ret;

                ++(disk->ref);
                fs_ret = disk->fs_dir_create_entry(disk, dir, name, attribute, dir_entry);
                --(disk->ref);

                return fs_ret;
            }
        }
    }

    return -1;
}

int disk_dir_create(struct dir *dir, const char *name, struct dir_entry *dir_entry)
{
    if (dir != NULL && name != NULL && dir_entry != NULL)
    {
        if (dir->disk >= KERNEL_DISK_ID_START && dir->disk <= KERNEL_DISK_ID_END)
        {
            struct disk *disk = find_disk_by_id(dir->disk);

            if (disk != NULL && disk->fs_dir_create != NULL)
            {
                int fs_ret;

                ++(disk->ref);
                fs_ret = disk->fs_dir_create(disk, dir, name, dir_entry);
                --(disk->ref);

                return fs_ret;
            }
        }
    }

    return -1;
}

int disk_fs_request(uint32_t disk_id, enum FS_REQUEST_TYPE type, void *params, void *ret)
{
    if (type < FS_REQUEST_TYPE_SIZE)
    {
        disk_id = (uint16_t)disk_id;

        if (disk_id >= KERNEL_DISK_ID_START && disk_id <= KERNEL_DISK_ID_END)
        {
            struct disk *disk = find_disk_by_id(disk_id);

            if (disk != NULL && disk->fs_request != NULL)
            {
                int fs_ret;

                ++(disk->ref);
                fs_ret = disk->fs_request(disk, type, params, ret);
                --(disk->ref);

                return fs_ret;
            }
        }
    }

    return -1;
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
            printk("fs type:\t %s\n", node->fs_type);
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
            if (node->total > (1 * MB))
            {
                len = printk(" %d", node->total / MB);
            }
            else
            {
                len = printk("| 1 <=");
            }
            put_space(len, 11);
            if (node->free > (1 * MB))
            {
                len = printk("| %d", node->free / MB);
            }
            else
            {
                len = printk("| 1 <=");
            }
            put_space(len, 11);
            if (node->used > (1 * MB))
            {
                len = printk("| %d", node->used / MB);
            }
            else
            {
                len = printk("| 1 <=");
            }
            put_space(len, 11);
            printk("\n");
            node = node->next;
        }
    }
}
