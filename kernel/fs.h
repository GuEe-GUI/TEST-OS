#include <fat32.h>

#define UNKNOWN_FS_TYPE "unknown file system"

static struct
{
    char *type;
    int (*check)(struct disk *disk);
    int (*format)(struct disk *disk);
} fs_opts[] =
{
    {
        .type = "fat32",
        .check = &fat32_check,
        .format = &fat32_format,
    },
};
