#include <fat32.h>
#include <kernel.h>
#include <malloc.h>
#include <rtc.h>
#include <string.h>
#include <types.h>

/*
 * Microsoft Extensible Firmware Initiative FAT32 File System Specification
 */

#define FAT_ENOENT  1
#define FAT_EIO     2
#define FAT_EINVAL  3
#define FAT_ENOSPC  4

struct fat32_extended_bpb
{
    uint32_t fat_sectors;
    uint16_t flags;
    uint16_t version;
    uint32_t rootdir_cluster;
    uint16_t fs_info;
    uint16_t boot_record_sectors;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t bootstrap_reserved;
    uint8_t boot_signature;
    uint32_t volume_serial_number;
    char volume_label[11];
    char fs_type[8];
} __attribute__((packed));

struct fat32_bpb
{
    uint8_t jmp_code[3];
    char os_name[8];
    uint16_t sector_size;
    uint8_t cluster_sectors;
    uint16_t reserved_sectors;
    uint8_t num_fats;
    uint16_t rootdir_length;
    uint16_t total_sectors;
    uint8_t media_type;

    /* fat32 benutzt fat32_extended_bpb.fat_sectors */
    uint16_t fat_sectors;

    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_large;
    struct fat32_extended_bpb extended;
    char boot_code[0x200 - 0x5a - 2];
    uint16_t magic;
} __attribute__((packed));

static const char boot_code[] __attribute__((aligned(1))) =
    "\x0e"          /* 05a:         push  cs */
    "\x1f"          /* 05b:         pop   ds */
    "\xbe\x77\x7c"  /*  write_msg:  mov   si, offset message_txt */
    "\xac"          /* 05f:         lodsb */
    "\x22\xc0"      /* 060:         and   al, al */
    "\x74\x0b"      /* 062:         jz    key_press */
    "\x56"          /* 064:         push  si */
    "\xb4\x0e"      /* 065:         mov   ah, 0eh */
    "\xbb\x07\x00"  /* 067:         mov   bx, 0007h */
    "\xcd\x10"      /* 06a:         int   10h */
    "\x5e"          /* 06c:         pop   si */
    "\xeb\xf0"      /* 06d:         jmp   write_msg */
    "\x32\xe4"      /*  key_press:  xor   ah, ah */
    "\xcd\x16"      /* 071:         int   16h */
    "\xcd\x19"      /* 073:         int   19h */
    "\xeb\xfe"      /*  foo:        jmp   foo */
    /* 077: message_txt: */
    "This is not a bootable disk\r\n";

struct fat32_fs_info
{
    union
    {
        struct
        {
            uint32_t signature1;
            uint8_t reserved1[480];
            uint32_t signature2;
            uint32_t free_clusters;      /* free cluster count.  -1 if unknown */
            uint32_t next_cluster;       /* most recently allocated cluster */
            uint8_t reserved2[12];
            uint32_t boot_sign;
        } __attribute__((packed));
        uint8_t buffer[512];
    };
} __attribute__((packed));

struct fat32_fs
{
    struct fat32_bpb bpb;
    uint32_t total_sectors;
    uint32_t first_data_cluster;
};

#define FAT_ATTRIB_READONLY 0x01
#define FAT_ATTRIB_HIDDEN   0x02
#define FAT_ATTRIB_SYSTEM   0x04
#define FAT_ATTRIB_VOLUME   0x08
#define FAT_ATTRIB_DIR      0x10
#define FAT_ATTRIB_ARCHIVE  0x20

struct fat_disk_dir_entry
{
    char name[8];
    char ext[3];
    uint8_t attrib;
    uint8_t data[14];
    uint16_t first_cluster;
    uint32_t size;
} __attribute__((packed));

struct fat32_dir_entry
{
    int index_in_dir;
    int dir_first_cluster;
    char name[13];
    int first_cluster;
    int attrib;
    uint32_t size;
};

struct fat32_dir
{
    struct fat32_fs *fs;
    struct fat_disk_dir_entry *entries;
    int first_cluster;
    int i;
    int num;
};

struct fat32_file
{
    struct fat32_fs *fs;
    uint32_t size;
    size_t blocklist_size;
    uint32_t *blocklist;
    struct fat32_dir_entry dir_entry;
};

size_t fat32_file_read(struct disk *disk, void *file, void *buffer, off_t offset, size_t length)
{
    return 0;
}

size_t fat32_file_write(struct disk *disk, void *file, const void *buffer, off_t offset, size_t length)
{
    return 0;
}

void *fat32_file_open(struct disk *disk, const char *path)
{
    return NULL;
}

int fat32_file_close(struct disk *disk, void *file)
{
    return 0;
}

int fat32_path_dir(struct disk *disk, const char *path)
{
    return 0;
}

int fat32_check(struct disk *disk)
{
    uint32_t total_sectors;
    uint32_t rootdir_sectors;
    uint32_t data_sectors;
    struct fat32_fs fs;
    struct fat32_fs_info fs_info;

    if (!disk->device_read(disk, 0, &fs.bpb, 1) || fs.bpb.magic != 0xaa55)
    {
        return -1;
    }

    if (fs.bpb.total_sectors != 0)
    {
        total_sectors = fs.bpb.total_sectors;
    }
    else
    {
        total_sectors = fs.bpb.total_sectors_large;
    }

    rootdir_sectors = (fs.bpb.rootdir_length * sizeof(struct fat32_dir_entry) + 511) / 512;

    data_sectors = total_sectors - fs.bpb.reserved_sectors
            - (fs.bpb.num_fats * fs.bpb.fat_sectors) - rootdir_sectors;
    data_sectors -= (fs.bpb.num_fats * fs.bpb.extended.fat_sectors);

    fs.total_sectors = total_sectors;
    fs.first_data_cluster = total_sectors - data_sectors;

    if ((disk->fs = malloc(sizeof(struct fat32_fs))) == NULL)
    {
        return -1;
    }

    memcpy(disk->fs, &fs, sizeof(struct fat32_fs));

    disk->fs_file_read = &fat32_file_read;
    disk->fs_file_write = &fat32_file_write;
    disk->fs_file_open = &fat32_file_open;
    disk->fs_file_close = &fat32_file_close;
    disk->fs_path_dir = &fat32_path_dir;
    disk->fs_request = NULL;
    disk->fs_type = "fat32";
    disk->fs_filesize = sizeof(struct fat32_file);

    if (fs.bpb.extended.boot_signature == 0x29)
    {
        disk->device_read(disk, 1, fs_info.buffer, 1);

        disk->free = fs_info.free_clusters * 512;
        disk->used = disk->total - disk->free;
    }
    else
    {
        disk->free = 0;
        disk->used = 0;
    }

    return 0;
}

int fat32_format(struct disk *disk)
{
    int i, j;
    uint32_t total_sectors = disk->total / 512;
    uint32_t first_data_sector;
    struct fat32_bpb bpb;
    struct fat32_fs_info fs_info;

    if (disk->fs != NULL)
    {
        free(disk->fs);
    }

    memcpy(&bpb.jmp_code[0], "\xeb\x58\x90", 3);
    memcpy(&bpb.os_name[0], "TEST OS ", 8);
    bpb.sector_size = 512;
    bpb.cluster_sectors = 1;
    bpb.reserved_sectors = 32;
    bpb.num_fats = 2;
    bpb.rootdir_length = 0;
    bpb.total_sectors = (total_sectors < 0x10000 ? (uint16_t)total_sectors : 0);
    bpb.media_type = 0xF8;
    bpb.fat_sectors = 0;
    bpb.sectors_per_track = 32;
    bpb.heads = 64;
    bpb.hidden_sectors = 0;
    bpb.total_sectors_large = (bpb.total_sectors == 0 ? total_sectors : 0);
    bpb.extended.fat_sectors =
        ({
            uint16_t root_entry_count = bpb.rootdir_length;
            uint16_t root_dir_sectors = ((root_entry_count * 32) + (bpb.sector_size - 1)) / bpb.sector_size;
            uint32_t tmp_val1 = total_sectors - (bpb.reserved_sectors + root_dir_sectors);
            uint32_t tmp_val2 = ((256UL * bpb.cluster_sectors) + bpb.num_fats) / 2;

            (tmp_val1 + (tmp_val2 - 1)) / tmp_val2;
        });
    bpb.extended.flags = 0;
    bpb.extended.version = 0;
    bpb.extended.rootdir_cluster = 2;
    bpb.extended.fs_info = 1;
    /* set to 0 as we do not create a boot sectors copy yet */
    bpb.extended.boot_record_sectors = 6;
    memset(&bpb.extended.reserved[0], 0, 12);
    bpb.extended.drive_number = 0x80;
    bpb.extended.bootstrap_reserved = 0;
    bpb.extended.boot_signature = 0x29;
    bpb.extended.volume_serial_number = (uint32_t)get_timestamp();
    memcpy(&bpb.extended.volume_label[0], "NO NAME    ", 11);
    memcpy(&bpb.extended.fs_type[0], "FAT32   ", 8);
    memset(&bpb.boot_code[0], 0, sizeof(bpb.boot_code));
    memcpy(&bpb.boot_code[0], boot_code, sizeof(boot_code));
    /* specification demands this values at this precise positions */
    bpb.magic = 0xAA55;

    disk->device_write(disk, 0, &bpb, 1);

    memset(fs_info.buffer, 0, 512);
    /* media_type copy */
    ((uint32_t *)fs_info.buffer)[0] = 0x0FFFFF00 + bpb.media_type;
    /* end of cluster chain marker */
    ((uint32_t *)fs_info.buffer)[1] = 0x0FFFFFFF;
    /* end of cluster chain marker for root directory at cluster 2 */
    ((uint32_t *)fs_info.buffer)[2] = 0x0FFFFFF8;

    /* write first sector of the fats */
    disk->device_write(disk, bpb.reserved_sectors, fs_info.buffer, 1);
    /* write first sector of the secondary fat(s) */
    for (j = 1; j < bpb.num_fats; ++j)
    {
        disk->device_write(disk, bpb.reserved_sectors + bpb.extended.fat_sectors * j, fs_info.buffer, 1);
    }

    /* reset previously written first 96 bits = 12 Bytes of the buffer */
    memset(fs_info.buffer, 0, 12);

    /* write additional sectors of the fats */
    disk->device_write(disk, bpb.reserved_sectors + 1, fs_info.buffer, 1);
    for (i = 1; i < bpb.extended.fat_sectors / 2; ++i)
    {
        disk->device_write(disk, bpb.reserved_sectors + i + 1, fs_info.buffer, 1);
    }

    /* write additional sectors of the secondary fat(s) */
    for (i = 1; i < bpb.num_fats; ++i)
    {
        for (j = 1; j < bpb.extended.fat_sectors; ++j)
        {
            disk->device_write(disk, bpb.reserved_sectors + j + bpb.extended.fat_sectors * i, fs_info.buffer, 1);
        }
    }

    memset(fs_info.buffer, 0, 512);
    fs_info.signature1 = 0x41615252; /* "RRaA" */
    fs_info.signature2 = 0x61417272; /* "rrAa" */
    fs_info.free_clusters = (total_sectors - ((bpb.extended.fat_sectors * bpb.num_fats) + bpb.reserved_sectors)) / bpb.cluster_sectors;
    fs_info.next_cluster = (bpb.reserved_sectors + (bpb.num_fats * bpb.extended.fat_sectors));
    if (fs_info.next_cluster % bpb.cluster_sectors)
    {
        fs_info.next_cluster = (fs_info.next_cluster / bpb.cluster_sectors) + 1;
    }
    else
    {
        fs_info.next_cluster = (fs_info.next_cluster / bpb.cluster_sectors);
    }
    fs_info.boot_sign = 0xAA550000;

    disk->device_write(disk, 1, fs_info.buffer, 1);

    memset(fs_info.buffer, 0, 512);
    first_data_sector = bpb.reserved_sectors + (bpb.num_fats * bpb.extended.fat_sectors);

    /* clear first root cluster */
    for (i = 0; i < bpb.cluster_sectors; ++i)
    {
        disk->device_write(disk, first_data_sector + i, fs_info.buffer, 1);
    }

    return fat32_check(disk);
}
