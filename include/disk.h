#ifndef _DISK_H_
#define _DISK_H_

#include <types.h>

#define KERNEL_DISK_ID_START    0x3a61  /* "a:" */
#define KERNEL_DISK_ID_END      0x3a7a  /* "z:" */

#define KERNEL_DISK_MAX_PATH    256

struct disk
{
    uint32_t id;
    char *name;
    size_t total;
    size_t free;
    size_t used;
    int ref;

    size_t (*device_read)(struct disk *disk, size_t offset, void *buffer, int count);
    size_t (*device_write)(struct disk *disk, size_t offset, const void *buffer, int count);
    size_t (*fs_file_read)(struct disk *disk, void *file, void *buffer, size_t offset, size_t length);
    size_t (*fs_file_write)(struct disk *disk, void *file, const void *buffer, size_t offset, size_t length);
    int (*fs_file_open)(struct disk *disk, void *fs, const char *path, void *file);
    int (*fs_file_close)(struct disk *disk, void *file);

    void *device;
    void *fs;
    char *fs_type;
    size_t fs_filesize;

    struct disk *next;
};

struct disk *disk_register(char *name);
void disk_format(uint32_t id, char *fs_type);
void init_disk();
void print_disk(uint32_t disk_id);

#endif
