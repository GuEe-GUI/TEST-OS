#include <fat32.h>
#include <kernel.h>
#include <malloc.h>
#include <string.h>
#include <types.h>

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
    union
    {
        struct fat32_extended_bpb fat32;
        uint8_t bytes[474];
    } extended;
    uint16_t magic;
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
    struct fat_disk_dir_entry* entries;
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

int fat32_check(struct disk *disk)
{
    uint32_t total_sectors;
    uint32_t rootdir_sectors;
    uint32_t data_sectors;
    struct fat32_fs fs;

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
    data_sectors -= (fs.bpb.num_fats * fs.bpb.extended.fat32.fat_sectors);

    fs.total_sectors = total_sectors;
    fs.first_data_cluster = total_sectors - data_sectors;

    if ((disk->fs = malloc(sizeof(struct fat32_fs))) == NULL)
    {
        return -1;
    }

    memcpy(disk->fs, &fs, sizeof(struct fat32_fs));

    disk->fs_type = "fat32";
    disk->fs_filesize = sizeof(struct fat32_file);

    return 0;
}

static uint8_t calc_cluster_size(uint16_t bytes)
{
    uint8_t sec_per_cluster = 0;
    uint16_t sec_size = 512;

    sec_per_cluster = (uint8_t) (bytes / sec_size);

    if (sec_per_cluster == 0)
    {
        return 1;
    }

    if (IS_POWER_OF_2(sec_per_cluster) != 0)
    {
        /* round down to power of 2 */
        uint32_t sec_per_cluster_new = 1U << 31;
        uint32_t sec_per_cluster_old = sec_per_cluster;

        while (sec_per_cluster_old < sec_per_cluster_new)
        {
            sec_per_cluster_new >>= 1;
        }

        sec_per_cluster = (uint8_t)sec_per_cluster_new;
    }

    if (sec_per_cluster > 128 || sec_per_cluster * sec_size > 32 * 1024)
    {
        return 1;
    }

    return sec_per_cluster;
}

/* Microsoft Extensible Firmware Initiative FAT32 File System Specification (PDF) */
int fat32_format(struct disk *disk)
{
    int i, j;
    uint8_t buffer[512];
    uint8_t num_fats;
    uint8_t media_type;
    uint8_t sec_per_cluster;
    uint8_t sectors_per_cluster;
    uint16_t bytes_per_sec;
    uint16_t type_sec_per_cluster;
    uint16_t reserved_sectors_count;
    uint16_t root_entry_count;
    uint32_t total_sectors;
    uint32_t fat_size;
    uint32_t first_data_sector;
    uint32_t fsi_free_count = 0;
    uint32_t fsi_nxt_free = 0;
    uint32_t num_sectors;
    uint32_t fat_invalid = 3;

    if (disk->fs != NULL)
    {
        free(disk->fs);
    }

    memset(buffer, 0, 512);

    num_sectors = disk->total / 512;
    type_sec_per_cluster =
        ({
            uint64_t vol_size = num_sectors;
            uint16_t result = fat_invalid << 8;

            if (vol_size < 524288)
            {
                result = (1 << 8) + calc_cluster_size(4096);
            }
            else if (vol_size < 4194304L)
            {
                result = (2 << 8) + calc_cluster_size(2048);
            }
            else if (vol_size < 16777216)
            {
                result = (2 << 8) + calc_cluster_size(4096);
            }
            else if (vol_size < 33554432)
            {
                result = (2 << 8) + calc_cluster_size(8192);
            }
            else if (vol_size < 67108864)
            {
                result = (2 << 8) + calc_cluster_size(16384);
            }
            else if (vol_size < 0xFFFFFFFF)
            {
                result = (2 << 8) + calc_cluster_size(32768);
            }
            result;
        });
    sectors_per_cluster = (uint8_t)type_sec_per_cluster;

    fat_size = 0;

    if ((uint8_t)(type_sec_per_cluster >> 8) == fat_invalid)
    {
        return -1;
    }

    /* BS_jmpBoot */
    buffer[0] = buffer[1] = buffer[2] = 0;

    /* BS_OEMName */
    memcpy(&(buffer[0x3]), "NO NAME ", 8);

    /* BPB_BytsPerSec */
    buffer[0x0B] = (uint8_t)512;
    buffer[0x0C] = (uint8_t)(512 >> 8);
    bytes_per_sec = 512;

    /* BPB_SecPerClus */
    buffer[0x0D] = sectors_per_cluster;
    sec_per_cluster = sectors_per_cluster;

    /* BPB_RsvdSecCnt */
    buffer[0x0E] = 32;
    buffer[0x0F] = 0;
    reserved_sectors_count = 32;

    /* BPB_NumFATs */
    buffer[0x10] = 2;
    num_fats = 2;

    /* BPB_RootEntCnt */
    buffer[0x11] = buffer[0x12] = 0;
    root_entry_count = 0;

    /* BPB_TotSec16 */
    if (num_sectors < 0x10000)
    {
        buffer[0x13] = (uint8_t)num_sectors;
        buffer[0x14] = (uint8_t)(num_sectors >> 8);
    }
    else
    {
        buffer[0x13] = buffer[0x14] = 0;
    }
    total_sectors = num_sectors;

    /* BPB_Media */
    buffer[0x15] = 0xF8;
    media_type = 0xF8;

    fat_size =
        ({
            uint16_t root_dir_sectors = ((root_entry_count * 32) + (bytes_per_sec - 1)) / bytes_per_sec;
            uint32_t tmp_val1 = total_sectors - (reserved_sectors_count + root_dir_sectors);
            uint32_t tmp_val2 = (256UL * sec_per_cluster) + num_fats;

            tmp_val2 /= 2;

            (tmp_val1 + (tmp_val2 - 1)) / tmp_val2;
        });

    /* BPB_FATSz16 */
    buffer[0x16] = buffer[0x17] = 0;

    /* BPB_SecPerTrk */
    buffer[0x18] = buffer[0x19] = 0;

    /* BPB_NumHeads */
    buffer[0x1A] = buffer[0x1B] = 0;

    /* BPB_HiddSec */
    buffer[0x1C] = buffer[0x1D] = buffer[0x1E] = buffer[0x1F] = 0;

    /* BPB_TotSec32 */
    if (num_sectors < 0x10000)
    {
        buffer[0x20] = buffer[0x21] = buffer[0x22] = buffer[0x23] = 0;
    }
    else
    {
        buffer[0x20] = (uint8_t)total_sectors;
        buffer[0x21] = (uint8_t)(total_sectors >> 8);
        buffer[0x22] = (uint8_t)(total_sectors >> 16);
        buffer[0x23] = (uint8_t)(total_sectors >> 24);
    }

    /* BPB_FATSz32 */
    buffer[0x24] = (uint8_t)fat_size;
    buffer[0x25] = (uint8_t)(fat_size >> 8);
    buffer[0x26] = (uint8_t)(fat_size >> 16);
    buffer[0x27] = (uint8_t)(fat_size >> 24);

    /* BPB_ExtFlags */
    buffer[0x28] = buffer[0x29] = 0;

    /* BPB_FSVer */
    buffer[0x2A] = buffer[0x2B] = 0;

    /* BPB_RootClus */
    buffer[0x2C] = 2;
    buffer[0x2D] = buffer[0x2E] = buffer[0x2F] = 0;

    /* BPB_FSInfo */
    buffer[0x30] = 1;
    buffer[0x31] = 0;

    /* BPB_BkBootSec, set to 0 as we do not create a boot sectors copy yet */
    buffer[0x32] = buffer[0x33] = 0;

    /* BPB_Reserved */
    memset(&(buffer[0x34]), 0, 12);

    /* BS_DrvNum */
    buffer[0x40] = 0x80;

    /* BS_Reserved1 */
    buffer[0x41] = 0;

    /* BS_BootSig */
    buffer[0x42] = 0x29;

    /* BS_VolID */
    buffer[0x43] = buffer[0x44] = buffer[0x45] = buffer[0x46] = 0;

    /* BS_VolLab */
    memcpy(&(buffer[0x47]), "NO NAME    ", 11);

    /* BS_FilSysType */
    memcpy(&(buffer[0x52]), "FAT32   ", 8);

    /* specification demands this values at this precise positions */
    buffer[0x1FE] = 0x55;
    buffer[0x1FF] = 0xAA;

    disk->device_write(disk, 0, buffer, 1);

    memset(buffer, 0, 512);
    /* media_type Copy */
    ((uint32_t *)buffer)[0] = 0x0FFFFF00 + media_type;
    /* end of cluster chain marker */
    ((uint32_t *)buffer)[1] = 0x0FFFFFFF;
    /* end of cluster chain marker for root directory at cluster 2 */
    ((uint32_t *)buffer)[2] = 0x0FFFFFF8;

    /* write first sector of the fats */
    disk->device_write(disk, reserved_sectors_count, buffer, 1);
    /* write first sector of the secondary fat(s) */
    for (j = 1; j < num_fats; ++j)
    {
        disk->device_write(disk, reserved_sectors_count + fat_size * j, buffer, 1);
    }

    /* reset previously written first 96 bits = 12 Bytes of the buffer */
    memset(buffer, 0, 12);

    /* write additional sectors of the fats */
    disk->device_write(disk, reserved_sectors_count + 1, buffer, 1);
    for (i = 1; i < fat_size / 2; ++i)
    {
        disk->device_write(disk, reserved_sectors_count + i + 1, buffer, 1);
    }

    /* write additional sectors of the secondary fat(s) */
    for (j = 1; j < num_fats; ++j)
    {
        for (i = 1; i < fat_size; ++i)
        {
            disk->device_write(disk, reserved_sectors_count + i + fat_size * j, buffer, 1);
        }
    }

    memset(buffer, 0, 512);
    /* FSI_LeadSig */
    *((uint32_t *)&buffer[0]) = 0x41615252;

    /* FSI_StrucSig */
    *((uint32_t *)&buffer[484]) = 0x61417272;

    /* FSI_Free_Count */
    fsi_free_count = (total_sectors - ((fat_size * num_fats) + reserved_sectors_count)) / sec_per_cluster;
    buffer[488] = (uint8_t)fsi_free_count;
    buffer[489] = (uint8_t)(fsi_free_count >> 8);
    buffer[490] = (uint8_t)(fsi_free_count >> 16);
    buffer[491] = (uint8_t)(fsi_free_count >> 24);

    /* FSI_Nxt_Free */
    fsi_nxt_free = (reserved_sectors_count + (num_fats * fat_size));
    if (fsi_nxt_free % sec_per_cluster)
    {
        fsi_nxt_free = (fsi_nxt_free / sec_per_cluster) + 1;
    }
    else
    {
        fsi_nxt_free = (fsi_nxt_free / sec_per_cluster);
    }
    buffer[492] = (uint8_t)fsi_nxt_free;
    buffer[493] = (uint8_t)(fsi_nxt_free >> 8);
    buffer[494] = (uint8_t)(fsi_nxt_free >> 16);
    buffer[495] = (uint8_t)(fsi_nxt_free >> 24);

    /* FSI_TrailSig */
    buffer[508] = 0x00;
    buffer[509] = 0x00;
    buffer[510] = 0x55;
    buffer[511] = 0xAA;

    disk->device_write(disk, 1, buffer, 1);

    first_data_sector = reserved_sectors_count + (num_fats * fat_size);
    memset(buffer, 0, 512);

    /* clear first root cluster */
    for (i = 0; i < sec_per_cluster; ++i)
    {
        disk->device_write(disk, first_data_sector + i, buffer, 1);
    }

    return fat32_check(disk);;
}

size_t fat32_read(struct disk *disk, uint64_t start, size_t size, void *buffer)
{
    return 0;
}

size_t fat32_write(struct disk *disk, size_t size, const void *buffer)
{
    return 0;
}
