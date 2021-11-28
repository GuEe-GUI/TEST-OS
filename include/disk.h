#ifndef _DISK_H_
#define _DISK_H_

#include <types.h>

#define KERNEL_DISK_ID_START 'a'
#define KERNEL_DISK_ID_END   'z'

struct disk
{
    char id;
    char *name;
    char *fs_type;
    size_t total;
    size_t free;
    size_t used;
    int ref;
    int (*read)(struct disk *disk, int offset, void *buffer, int count);
    int (*write)(struct disk *disk, int offset, void *buffer, int count);
    void *device;

    struct disk *next;
};

void disk_register(char *name, int (*read)(), int (*write)(), void *device);
void disk_format(char id, char *fs_type);
void init_disk();
void print_disk(int disk_id);

#endif
