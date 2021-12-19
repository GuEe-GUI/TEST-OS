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

    size_t (*device_read)(struct disk *disk, off_t offset, void *buffer, int count);
    size_t (*device_write)(struct disk *disk, off_t offset, const void *buffer, int count);
    size_t (*fs_file_read)(struct disk *disk, void *file, void *buffer, off_t offset, size_t length);
    size_t (*fs_file_write)(struct disk *disk, void *file, const void *buffer, off_t offset, size_t length);
    void *(*fs_file_open)(struct disk *disk, const char *path);
    int (*fs_file_close)(struct disk *disk, void *file);
    int (*fs_path_dir)(struct disk *disk, const char *path);
    int (*fs_request)(struct disk *disk, void *params);

    void *device;
    void *fs;
    char *fs_type;
    size_t fs_filesize;

    struct disk *next;
};

struct file
{
    uint32_t fid;
    uint32_t disk;
    off_t offset;

    uint8_t *buffer;
    size_t buffer_size;

    void *fs_file;
};

struct disk *disk_register(char *name);
void disk_format(uint32_t id, char *fs_type);
void init_disk();
size_t disk_file_read(struct file *file, void *buffer, off_t offset, size_t length);
size_t disk_file_write(struct file *file, const void *buffer, off_t offset, size_t length);
void *disk_file_open(const char *path);
int disk_file_close(struct file *file);
int disk_path_dir(const char *path);
int disk_fs_request(uint32_t disk_id, void *params);
void print_disk(uint32_t disk_id);

#endif /* _DISK_H_ */
