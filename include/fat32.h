#ifndef _FAT32_H_
#define _FAT32_H_

#include <disk.h>

int fat32_check(struct disk *disk);
int fat32_format(struct disk *disk);

#endif
