#ifndef _DISK_H_
#define _DISK_H_

#include <types.h>

#define KERNEL_DISK_ID_START    0x3a61  /* "a:" */
#define KERNEL_DISK_ID_END      0x3a7a  /* "z:" */
#define KERNEL_DISK_ID_LEN      (sizeof(uint16_t))

#define KERNEL_DISK_MAX_PATH    128

#define DISK_PATH_ID(path)      (*(uint16_t *)path)

struct file
{
    uint32_t id;
    uint32_t disk;
    char path[KERNEL_DISK_MAX_PATH];

    void *fs_file;
};

enum DIR_ENTRY_ATTRIB
{
    DIR_ENTRY_READONLY  = 0x01,
    DIR_ENTRY_HIDDEN    = 0x02,
    DIR_ENTRY_SYSTEM    = 0x04,
    DIR_ENTRY_VOLUME    = 0x08,
    DIR_ENTRY_DIR       = 0x10,
    DIR_ENTRY_ARCHIVE   = 0x20,
};

struct dir_entry
{
    uint32_t id;
    uint32_t disk;
    char name[KERNEL_DISK_MAX_PATH];

    enum DIR_ENTRY_ATTRIB attribute;

    void *fs_dir_entry;
};

struct dir
{
    uint32_t id;
    uint32_t disk;
    char path[KERNEL_DISK_MAX_PATH];

    void *fs_dir;
};

enum FS_REQUEST_TYPE
{
    FS_FILE_DELETE = 0,
    FS_FILE_RENAME,
    FS_FILE_SIZE,
    FS_FILE_CREATE_DATE,
    FS_FILE_MODIFY_DATE,
    FS_FILE_ATTR,

    FS_DIR_DELETE,
    FS_DIR_RENAME,
    FS_DIR_SIZE,
    FS_DIR_CREATE_DATE,
    FS_DIR_MODIFY_DATE,
    FS_DIR_ATTR,

    FS_DIR_ENTRY_SIZE,
    FS_DATA_SYNC,

    FS_REQUEST_CUSTOM,

    FS_REQUEST_TYPE_SIZE
};

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

    int (*fs_file_open)(struct disk *disk, const char *path, struct file *file);
    int (*fs_file_close)(struct disk *disk, struct file* file);
    size_t (*fs_file_read)(struct disk *disk, struct file *file, void *buffer, off_t offset, size_t length);
    size_t (*fs_file_write)(struct disk *disk, struct file *file, const void *buffer, off_t offset, size_t length);
#define FILE_SEEK_SET 0
#define FILE_SEEK_CUR 1
#define FILE_SEEK_END 2
    int (*fs_file_seek)(struct disk *disk, struct file *file, off_t offset, int whence);
    int (*fs_dir_open)(struct disk *disk, const char *path, struct dir *dir);
    int (*fs_dir_close)(struct disk *disk, struct dir *dir);
    int (*fs_dir_read)(struct disk *disk, struct dir *dir, struct dir_entry *entry);
    int (*fs_dir_create_entry)(struct disk *disk, struct dir *dir, const char *name, enum DIR_ENTRY_ATTRIB attribute, struct dir_entry *dir_entry);
    int (*fs_dir_create)(struct disk *disk, struct dir *dir, const char *name, struct dir_entry *dir_entry);
    int (*fs_request)(struct disk *disk, enum FS_REQUEST_TYPE type, void *params, void *ret);

    void *device;
    void *fs;
    char *fs_type;

    struct disk *next;
};

struct disk *disk_register(char *name);
void disk_format(uint32_t id, char *fs_type);
void init_disk(void);
int disk_file_open(const char *path, struct file *file);
int disk_file_close(struct file* file);
size_t disk_file_read(struct file *file, void *buffer, off_t offset, size_t length);
size_t disk_file_write(struct file *file, const void *buffer, off_t offset, size_t length);
int disk_file_seek(struct file *file, off_t offset, int whence);
int disk_dir_open(const char *path, struct dir *dir);
int disk_dir_close(struct dir *dir);
int disk_dir_read(struct dir *dir, struct dir_entry *entry);
int disk_dir_create_entry(struct dir *dir, const char *name, enum DIR_ENTRY_ATTRIB attribute, struct dir_entry *dir_entry);
int disk_dir_create(struct dir *dir, const char *name, struct dir_entry *dir_entry);
int disk_fs_request(uint32_t disk_id, enum FS_REQUEST_TYPE type, void *params, void *ret);
void print_disk(uint32_t disk_id);

#endif /* _DISK_H_ */
