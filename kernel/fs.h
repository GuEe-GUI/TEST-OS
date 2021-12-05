#include <fat32.h>

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
