#include <ctype.h>
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
            uint32_t free_clusters; /* free cluster count */
            uint32_t next_cluster;  /* most recently allocated cluster */
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

#define FAT_ATTRIB_READONLY DIR_ENTRY_READONLY
#define FAT_ATTRIB_HIDDEN   DIR_ENTRY_HIDDEN
#define FAT_ATTRIB_SYSTEM   DIR_ENTRY_SYSTEM
#define FAT_ATTRIB_VOLUME   DIR_ENTRY_VOLUME
#define FAT_ATTRIB_DIR      DIR_ENTRY_DIR
#define FAT_ATTRIB_ARCHIVE  DIR_ENTRY_ARCHIVE

struct fat32_disk_dir_entry
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
    struct fat32_disk_dir_entry *entries;
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

#define NUM_CLUSTERS(x, cluster_size) (((x) + (cluster_size) - 1) / (cluster_size))
#define SECTOR_BITS (512 * 8)
#define MIN(a,b) ((a) < (b) ? (a) : (b))

static uint8_t fat32_buf[1024];
static struct fat32_fs *fat32_buf_fs = NULL;
static uint32_t fat32_buf_sector = 0;
static int fat32_buf_changed = 0;

static int fat32_file_open(struct disk *disk, const char *path, struct file *file);
static int fat32_file_close(struct disk *disk, struct file *file);
static size_t fat32_file_read(struct disk *disk, struct file *file, void *buffer, off_t offset, size_t length);
static size_t fat32_file_write(struct disk *disk, struct file *file, const void *buffer, off_t offset, size_t length);
static int fat32_dir_open(struct disk *disk, const char *path, struct dir *dir);
static int fat32_dir_close(struct disk *disk, struct dir *dir);
static int fat32_dir_read(struct disk *disk, struct dir *dir, struct dir_entry *entry);
static int fat32_dir_create_entry(struct disk *disk, struct dir *dir, const char *name, enum DIR_ENTRY_ATTRIB attribute, struct dir_entry *dir_entry);
static int fat32_dir_create(struct disk *disk, struct dir *dir, const char *name, struct dir_entry *dir_entry);

static void short_fn_to_string(struct fat32_disk_dir_entry *entry, char *buf)
{
    int i;
    int pos = -1;
    int ext_start;

    /* 8 Zeichen Name in Kleinbuchstaben */
    for (i = 0; i < 8; ++i)
    {
        buf[i] = tolower(entry->name[i]);
        if (isprint(buf[i]))
        {
            pos = i;
        }
    }

    /* Punkt */
    buf[++pos] = '.';

    /* 3 Zeichen Erweiterung in Kleinbuchstaben */
    ext_start = pos + 1;
    for (i = 0; i < 3; ++i)
    {
        buf[ext_start + i] = tolower(entry->ext[i]);

        if (isprint(buf[ext_start + i]))
        {
            pos = ext_start + i + 1;
        }
    }

    /* Nullterminieren */
    buf[pos] = '\0';
}

static void string_to_short_fn(struct fat32_disk_dir_entry *entry, const char *buf)
{
    int i;
    const char *p;

    for (i = 0; i < 8; ++i)
    {
        switch (buf[i])
        {
        case '.':
            memset(&entry->name[i], ' ', 8 - i);
            p = &buf[i + 1];
            goto parse_ext;
        case '\0':
            memset(&entry->name[i], ' ', 8 - i);
            p = &buf[i];
            goto parse_ext;
        default:
            entry->name[i] = toupper(buf[i]);
            break;
        }
    }

    p = strchr(&buf[8], '.');
parse_ext:
    if (!p || !*p)
    {
        memset(entry->ext, ' ', 3);
        return;
    }

    for (i = 0; i < 3; ++i)
    {
        if (p[i] == '\0')
        {
            memset(&entry->ext[i], ' ', 3 - i);
            return;
        }
        entry->ext[i] = toupper(p[i]);
    }
}

static int is_free_entry(struct fat32_disk_dir_entry *entry)
{
    /* Die Datei existiert nicht */
    if (entry->name[0] == '\0')
    {
        return 1;
    }

    /* Die Datei ist geloescht */
    if (entry->name[0] == (char)0xE5)
    {
        return 1;
    }

    return 0;
}

static int is_valid_entry(struct fat32_disk_dir_entry *entry)
{
    /* Die Datei existiert nicht oder ist geloescht */
    if (is_free_entry(entry))
    {
        return 0;
    }

    /* Die Datentraegerbezeichnung interessiert uns nicht */
    if (entry->attrib & FAT_ATTRIB_VOLUME)
    {
        return 0;
    }

    /* . und .. werden ignoriert */
    if (entry->name[0] == '.')
    {
        return 0;
    }

    return 1;
}

static void dir_entry_from_disk(struct fat32_dir_entry *entry, struct fat32_disk_dir_entry *disk_entry)
{
    short_fn_to_string(disk_entry, entry->name);

    entry->first_cluster        = disk_entry->first_cluster;
    entry->size                 = disk_entry->size;
    entry->attrib               = disk_entry->attrib;
}

static void dir_entry_to_disk(struct fat32_file *file, struct fat32_disk_dir_entry *disk_entry)
{
    string_to_short_fn(disk_entry, file->dir_entry.name);

    disk_entry->first_cluster   = file->dir_entry.first_cluster;
    disk_entry->size            = file->size;
    disk_entry->attrib          = file->dir_entry.attrib;
}

static uint8_t *get_fat32_buffer(struct disk *disk, struct fat32_fs *fs, uint32_t sector)
{
    sector += fs->bpb.reserved_sectors;

    if (fs == fat32_buf_fs && sector == fat32_buf_sector)
    {
        return fat32_buf;
    }

    /* FIXME Write back changed entries before overwriting them */
    /* TODO Evtl. reicht memcpy und lesen nur eines Sektors */
    if (!disk->device_read(disk, sector, fat32_buf, 1024 / 512))
    {
        fat32_buf_fs = NULL;
        return NULL;
    }

    fat32_buf_fs = fs;
    fat32_buf_sector = sector;
    fat32_buf_changed = 0;

    return fat32_buf;
}

static uint32_t get_next_cluster_fat32(struct disk *disk, struct fat32_fs *fs, uint32_t cluster)
{
    uint8_t *buf;
    uint32_t fat32_sector = (cluster * sizeof(uint32_t)) / 512;
    uint32_t fat32_offset = (cluster * sizeof(uint32_t)) % 512;
    uint32_t value;

    if ((buf = get_fat32_buffer(disk, fs, fat32_sector)) == NULL)
    {
        return 0;
    }

    value = *((uint32_t*) &buf[fat32_offset]);

    if (value > 0xffffff5)
    {
        value |= 0xf0000000;
    }

    return value;
}

static int fat32_file_open_by_dir_entry(struct disk *disk, struct fat32_dir_entry *entry, struct fat32_file *file)
{
    struct fat32_fs *fs = disk->fs;
    uint32_t cluster_size = fs->bpb.cluster_sectors * 512;
    uint32_t cluster;
    uint32_t i;

    /* Dateistruktur anlegen */
    file->fs = fs;
    file->size = entry->size;
    file->dir_entry = *entry;

    /* Eine leere Datei braucht keine Blockliste */
    if (entry->first_cluster == 0)
    {
        file->blocklist = NULL;
        file->blocklist_size = 0;
        return 0;
    }

    /*
     * Blockliste anlegen und einlesen. Bei Groesse 0 (z.B. Verzeichnisse gehen
     * wir von unbekannter Dateigroesse aus und lassen die Blockliste dynamisch
     * wachsen.
     */
    if (file->size > 0)
    {
        file->blocklist_size = NUM_CLUSTERS(file->size, cluster_size);
    }
    else
    {
        file->blocklist_size = 1;
    }

    file->blocklist = malloc(file->blocklist_size * sizeof(uint32_t));
    file->blocklist[0] = entry->first_cluster;

    cluster = 0;
    i = 1;

    for (;;)
    {
        cluster = get_next_cluster_fat32(disk, fs, file->blocklist[i - 1]);

        /* Fehler und Dateiende */
        if (cluster == 0)
        {
            goto fail;
        }
        else if (cluster >= 0xfffffff8)
        {
            break;
        }

        /* Blocklist vergroessern, falls noetig */
        if (file->blocklist_size <= i)
        {
            file->blocklist_size *= 2;
            file->blocklist = realloc(file->blocklist, file->blocklist_size * sizeof(uint32_t));
        }

        /* In die Blockliste eintragen */
        file->blocklist[i] = cluster;

        i++;
    }

    /* Dateigroesse anpassen, wenn sie unbekannt war */
    if (file->size == 0)
    {
        file->size = i * cluster_size;
    }

    return 0;

fail:
    free(file->blocklist);
    file->blocklist = NULL;
    return -FAT_EIO;
}

static int fat32_rootdir32_open(struct disk *disk, struct fat32_dir *dir)
{
    struct fat32_fs *fs = disk->fs;
    struct fat32_file *fs_file = (struct fat32_file *)malloc(sizeof(struct fat32_file));
    struct fat32_extended_bpb *ext = &fs->bpb.extended;
    struct fat32_dir_entry dentry =
    {
        .size = 0,
        .first_cluster = ext->rootdir_cluster,
    };
    int ret;
    void *buf;
    struct file file;

    if (fs_file == NULL)
    {
        return -1;
    }

    file.fs_file = fs_file;

    if ((ret = fat32_file_open_by_dir_entry(disk, &dentry, fs_file)) < 0)
    {
        free(fs_file);
        return ret;
    }

    buf = malloc(fs_file->size);

    ret = fat32_file_read(disk, &file, buf, 0, fs_file->size);
    fat32_file_close(disk, &file);
    free(fs_file);

    if (ret < 0)
    {
        free(buf);
        return ret;
    }

    /* Rueckgabestruktur befuellen */
    dir->entries = buf;
    dir->fs = fs;
    dir->i = 0;
    dir->first_cluster = 0;
    dir->num = fs_file->size / sizeof(struct fat32_disk_dir_entry);

    return 0;
}

static int get_data_cluster_offset(struct fat32_fs *fs, uint32_t cluster_idx)
{
    uint32_t sector = fs->bpb.cluster_sectors * (cluster_idx - 2);

    return (fs->first_data_cluster + sector);
}

static int read_cluster(struct disk *disk, struct fat32_fs *fs, void *buf, uint32_t cluster_idx)
{
    if (!disk->device_read(disk, get_data_cluster_offset(fs, cluster_idx), buf, fs->bpb.cluster_sectors))
    {
        return -FAT_EIO;
    }

    return 0;
}

static int write_cluster(struct disk *disk, struct fat32_fs *fs, void *buf, uint32_t cluster_idx)
{
    if (!disk->device_write(disk, get_data_cluster_offset(fs, cluster_idx), buf, fs->bpb.cluster_sectors))
    {
        return -FAT_EIO;
    }

    return 0;
}

static int write_cluster_part(struct disk *disk, struct fat32_fs *fs, void *buf, uint32_t cluster_idx, size_t offset, size_t len)
{
    uint32_t cluster_size = fs->bpb.cluster_sectors * 512;
    uint8_t cluster[cluster_size];
    int ret;

    ASSERT(offset + len <= cluster_size);

    if ((ret = read_cluster(disk, fs, cluster, cluster_idx)) < 0)
    {
        return ret;
    }

    memcpy(cluster + offset, buf, len);

    if ((ret = write_cluster(disk, fs, cluster, cluster_idx)) < 0)
    {
        return ret;
    }

    return 0;
}

static int read_cluster_part(struct disk *disk, struct fat32_fs *fs, void *buf, uint32_t cluster_idx, size_t offset, size_t len)
{
    uint32_t cluster_size = fs->bpb.cluster_sectors * 512;
    uint8_t cluster[cluster_size];
    int ret;

    ASSERT(offset + len <= cluster_size);

    if ((ret = read_cluster(disk, fs, cluster, cluster_idx)) < 0)
    {
        return ret;
    }

    memcpy(buf, cluster + offset, len);

    return 0;
}

static uint32_t file_get_cluster(struct fat32_file *file, uint32_t file_cluster)
{
    if (file_cluster >= file->blocklist_size)
    {
        return 0;
    }

    return file->blocklist[file_cluster];
}

static int32_t fat32_file_rw(struct disk *disk, struct fat32_file *file, void *buf, uint32_t offset, size_t len, int is_write)
{
    struct fat32_fs *fs = disk->fs;
    uint32_t cluster_size = fs->bpb.cluster_sectors * 512;
    uint8_t *p = buf;
    size_t n = len;
    int ret;
    uint32_t file_cluster = offset / cluster_size;
    uint32_t disk_cluster;
    uint32_t offset_in_cluster;

    /* Funktionspointer fuer Lesen/Schreiben von Clustern */
    int (*rw_cluster)(struct disk *disk, struct fat32_fs *fs, void *buf, uint32_t cluster_idx);
    int (*rw_cluster_part)(struct disk *disk, struct fat32_fs *fs, void *buf, uint32_t cluster_idx, size_t offset, size_t len);

    if (is_write)
    {
        rw_cluster      = write_cluster;
        rw_cluster_part = write_cluster_part;
    }
    else
    {
        rw_cluster      = read_cluster;
        rw_cluster_part = read_cluster_part;
    }

    /* Angefangener Cluster am Anfang */
    offset_in_cluster = offset & (cluster_size - 1);
    if (offset_in_cluster)
    {
        uint32_t count = MIN(n, cluster_size - offset_in_cluster);

        if ((disk_cluster = file_get_cluster(file, file_cluster++)) == 0)
        {
            return -FAT_EIO;
        }

        if ((ret = rw_cluster_part(disk, fs, p, disk_cluster, offset_in_cluster, count)) < 0)
        {
            return ret;
        }

        p += count;
        n -= count;
    }

    /* Eine beliebige Anzahl von ganzen Clustern in der Mitte */
    while (n >= cluster_size)
    {
        if ((disk_cluster = file_get_cluster(file, file_cluster++)) == 0)
        {
            return -FAT_EIO;
        }

        if ((ret = rw_cluster(disk, fs, p, disk_cluster)) < 0)
        {
            return ret;
        }

        p += cluster_size;
        n -= cluster_size;
    }

    /* Und wieder ein angefangener Cluster am Ende */
    if (n)
    {
        if ((disk_cluster = file_get_cluster(file, file_cluster)) == 0)
        {
            return -FAT_EIO;
        }

        if ((ret = rw_cluster_part(disk, fs, p, disk_cluster, 0, n)) < 0)
        {
            return ret;
        }
    }

    return len;
}

static int commit_fat32_buffer(struct disk *disk, struct fat32_fs *fs)
{
    int i;
    uint32_t sector;

    if (fs != fat32_buf_fs)
    {
        return 0;
    }

    sector = fat32_buf_sector;
    for (i = 0; i < fs->bpb.num_fats; i++)
    {
        /* TODO Evtl. reicht schreiben nur eines Sektors */
        if (!disk->device_write(disk, sector, fat32_buf, 1024 / 512))
        {
            return -FAT_EIO;
        }

        sector += fs->bpb.extended.fat_sectors;
    }

    fat32_buf_changed = 0;

    return 0;
}

static int fat32_dir_update_rootdir_entry(struct disk *disk, struct fat32_file *file)
{
    struct fat32_fs *fs = file->fs;
    struct fat32_disk_dir_entry dentry = { 0 };

    int64_t rootdir_offset = fs->bpb.reserved_sectors + (fs->bpb.num_fats * fs->bpb.fat_sectors);
    uint32_t offset = rootdir_offset * 512 + file->dir_entry.index_in_dir * sizeof(struct fat32_disk_dir_entry);

    if (!disk->device_write(disk, offset / 512, &dentry, sizeof(struct fat32_disk_dir_entry) / 512))
    {
        return -FAT_EIO;
    }

    dir_entry_to_disk(file, &dentry);

    if (!disk->device_write(disk, offset / 512, &dentry, sizeof(struct fat32_disk_dir_entry) / 512))
    {
        return -FAT_EIO;
    }

    return 0;
}

static int fat32_dir_update_subdir_entry(struct disk *disk, struct fat32_file *file)
{
    struct fat32_dir_entry fs_dir_entry =
    {
        .first_cluster = file->dir_entry.dir_first_cluster,
        .size = 0,
        .attrib = FAT_ATTRIB_DIR,
    };
    struct fat32_disk_dir_entry file_dentry;
    struct fat32_file *fs_dir_file = (struct fat32_file *)malloc(sizeof(struct fat32_file));
    struct file dir_file;
    int ret = -1;
    uint32_t offset = file->dir_entry.index_in_dir * sizeof(struct fat32_disk_dir_entry);

    dir_file.fs_file = fs_dir_file;

    if (fs_dir_file == NULL)
    {
        goto fail;
    }

    if ((ret = fat32_file_open_by_dir_entry(disk, &fs_dir_entry, fs_dir_file)) < 0)
    {
        goto fail;
    }

    /* Bisherigen Verzeichniseintrag laden, falls vorhanden */
    if (offset < file->size)
    {
        if ((ret = fat32_file_read(disk, &dir_file, &file_dentry, offset, sizeof(struct fat32_disk_dir_entry))) < 0)
        {
            goto fail;
        }
    }
    else
    {
        memset(&file_dentry, 0, sizeof(file_dentry));
    }

    /* Felder aktualisieren */
    dir_entry_to_disk(file, &file_dentry);

    if ((ret = fat32_file_write(disk, &dir_file, &file_dentry, offset, sizeof(struct fat32_disk_dir_entry))) < 0)
    {
        goto fail;
    }

    fat32_file_close(disk, &dir_file);
    free(fs_dir_file);

    return 0;

fail:
    if (fs_dir_file != NULL)
    {
        free(fs_dir_file);
    }
    return ret;
}

static int fat32_dir_update_entry(struct disk *disk, struct fat32_file *file)
{
    if (file->dir_entry.dir_first_cluster == 0)
    {
        return fat32_dir_update_rootdir_entry(disk, file);
    }
    else
    {
        return fat32_dir_update_subdir_entry(disk, file);
    }
}

static int fat32_extend_file(struct disk *disk, struct fat32_file *file, uint32_t new_size)
{
    struct fat32_fs *fs = file->fs;
    uint32_t cluster_size = fs->bpb.cluster_sectors * 512;
    size_t old_blocklist_size = file->blocklist_size;
    size_t new_blocklist_size = NUM_CLUSTERS(new_size, cluster_size);

    /* Ersten Cluster der Kette allozieren */
    if (file->blocklist == NULL)
    {
        old_blocklist_size = 1;
        file->blocklist_size = 1;
        file->blocklist = malloc(sizeof(*file->blocklist));
        file->blocklist[0] = file->dir_entry.first_cluster;
    }

    /* Wenn noetig, neue Cluster allozieren */
    if (new_blocklist_size != old_blocklist_size)
    {
        uint32_t i, j;
        uint32_t free_index = 0;
        uint32_t num_data_clusters = NUM_CLUSTERS(fs->total_sectors - fs->first_data_cluster, fs->bpb.cluster_sectors);

        /* Blockliste vergroessern */
        file->blocklist = realloc(file->blocklist, new_blocklist_size * sizeof(uint32_t));
        file->blocklist_size = new_blocklist_size;

        /* Neue Eintraege der Blockliste befuellen */
        for (i = old_blocklist_size; i < new_blocklist_size; ++i)
        {
            /* Freien Cluster suchen und einhaengen */
            for (j = free_index; j < num_data_clusters; ++j)
            {
                if (get_next_cluster_fat32(disk, fs, j) == 0) {

                    /* Neue EOF-Markierung */
                    file->blocklist[i] = j;
                    return -FAT_EINVAL;
                }
            }

            return -FAT_ENOSPC;
        }

        /* Und zuletzt veraenderte Eintraege zurueckschreiben */
        commit_fat32_buffer(disk, fs);
    }

    /* Dateigroesse anpassen und ggf. neuen ersten Cluster eintragen */
    file->size = new_size;

    if ((file->dir_entry.attrib & FAT_ATTRIB_DIR) == 0)
    {
        fat32_dir_update_entry(disk, file);
    }

    return 0;
}

static int fat32_subdir_open_by_dir_entry(struct disk *disk, struct fat32_dir_entry *dentry, struct fat32_dir *subdir)
{
    struct fat32_file *fs_file = (struct fat32_file *)malloc(sizeof(struct fat32_file));
    struct file file;
    void *buf;
    int first_cluster;
    int ret = -1;

    if (fs_file == NULL)
    {
        goto fail;
    }

    if ((ret = fat32_file_open_by_dir_entry(disk, dentry, fs_file)) < 0)
    {
        goto fail;
    }

    file.fs_file = fs_file;

    /*
     * Ein Verzeichnis ohne Cluster ist ein korruptes FS, fehlschlagen ist also
     * in Ordnung. Wir sollten nur nicht auf fs_file->blocklist[0] zugreifen.
     */
    if (fs_file->blocklist == NULL)
    {
        fat32_file_close(disk, &file);
        free(file.fs_file);
        ret = -FAT_ENOENT;
        goto fail;
    }

    buf = malloc(fs_file->size);
    first_cluster = fs_file->blocklist[0];

    ret = fat32_file_read(disk, &file, buf, 0, fs_file->size);
    fat32_file_close(disk, &file);
    free(file.fs_file);
    if (ret < 0)
    {
        free(buf);
        goto fail;
    }

    /* Rueckgabestruktur befuellen */
    subdir->entries = buf;
    subdir->fs = disk->fs;
    subdir->i = 0;
    subdir->first_cluster = first_cluster;
    subdir->num = fs_file->size / sizeof(struct fat32_disk_dir_entry);

    return 0;

fail:
    if (fs_file != NULL)
    {
        free(fs_file);
    }
    return ret;
}

static int fat32_subdir_open(struct disk *disk, struct dir *dir, const char *path, struct fat32_dir *subdir)
{
    struct fat32_dir_entry fs_dir_entry;
    struct dir_entry dir_entry;
    int ret;

    dir_entry.fs_dir_entry = &fs_dir_entry;

    /* Vaterverzeichnis nach Unterverzeichnis durchsuchen */
    do {
        if ((ret = fat32_dir_read(disk, dir, &dir_entry)) < 0)
        {
            return ret;
        }

        if ((ret >= 0) && !strcmp(fs_dir_entry.name, path))
        {
            goto found;
        }
    } while (ret >= 0);

    return -FAT_ENOENT;

    /* Verzeichnis als Datei oeffnen und Eintraege laden */
found:
    return fat32_subdir_open_by_dir_entry(disk, &fs_dir_entry, subdir);
}

static int fat32_file_open(struct disk *disk, const char *path, struct file *file)
{
    struct dir dir = { 0 };
    struct dir_entry dir_entry = { 0 };
    struct fat32_dir_entry *fs_dir_entry = (struct fat32_dir_entry *)malloc(sizeof(struct fat32_dir_entry));
    int ret = -1;
    int len;
    char dirname[KERNEL_DISK_MAX_PATH];
    char *p;


    if (fs_dir_entry == NULL)
    {
        goto fail;
    }

    file->fs_file = (struct fat32_file *)malloc(sizeof(struct fat32_file));

    if (file->fs_file == NULL)
    {
        goto fail;
    }

    /* Letzten Pfadtrenner suchen */
    if ((p = strrchr(path, '/')) == NULL)
    {
        ret = -FAT_EINVAL;
        goto fail;
    }

    dir_entry.fs_dir_entry = fs_dir_entry;
    len = p - path + 1;

    memcpy(dirname, path, len);
    dirname[len] = '\0';

    if ((ret = fat32_dir_open(disk, dirname, &dir)) < 0)
    {
        goto fail;
    }

    /* Alles vor dem Dateinamen wegwerfen */
    path = p + 1;

    /* Verzeichnis nach Dateinamen durchsuchen */
    do {
        if ((ret = fat32_dir_read(disk, &dir, &dir_entry)) >= 0 && !strcmp(fs_dir_entry->name, path))
        {
            break;
        }
    } while (ret >= 0);

    /* Datei oeffnen oder -ENOENT zurueckgeben, wenn sie nicht existiert */
    if (ret < 0)
    {
        fat32_dir_close(disk, &dir);
        ret = -FAT_ENOENT;
        goto fail;
    }

    /* Verzeichnis wieder schliessen */
    if ((ret = fat32_dir_close(disk, &dir)) < 0)
    {
        goto fail;
    }

    if ((ret = fat32_file_open_by_dir_entry(disk, fs_dir_entry, file->fs_file)) < 0)
    {
        goto fail;
    }

    goto success;

fail:
    if (file->fs_file != NULL)
    {
        free(file->fs_file);
        file->fs_file = NULL;
    }

success:
    if (dir.fs_dir != NULL)
    {
        free(dir.fs_dir);
    }
    if (fs_dir_entry != NULL)
    {
        free(fs_dir_entry);
    }
    return ret;
}

static int fat32_file_close(struct disk *disk, struct file *file)
{
    struct fat32_file *fs_file = file->fs_file;

    if (fs_file != NULL)
    {
        if (fs_file->blocklist != NULL)
        {
            free(fs_file->blocklist);
            fs_file->blocklist = NULL;
        }
    }

    return 0;
}

static size_t fat32_file_read(struct disk *disk, struct file *file, void *buffer, off_t offset, size_t length)
{
    struct fat32_file *fs_file = file->fs_file;

    /* Bei Ueberlaenge Fehler zurueckgeben */
    if (offset + length > fs_file->size)
    {
        return 0;
    }

    return fat32_file_rw(disk, fs_file, buffer, offset, length, 0);
}

static size_t fat32_file_write(struct disk *disk, struct file *file, const void *buffer, off_t offset, size_t length)
{
    struct fat32_file *fs_file = file->fs_file;

    /* Datei erweitern, falls noetig */
    if (offset + length > fs_file->size)
    {
        int ret = fat32_extend_file(disk, fs_file, offset + length);

        if (ret < 0)
        {
            return ret;
        }
    }

    return fat32_file_rw(disk, fs_file, (void *)buffer, offset, length, 1);
}

static int fat32_dir_open(struct disk *disk, const char *path, struct dir *dir)
{
    int ret = -1;
    char open_path[KERNEL_DISK_MAX_PATH];
    char *p;
    struct fat32_dir subdir;

    if ((dir->fs_dir = malloc(sizeof(struct fat32_dir))) == NULL)
    {
        goto out;
    }

    if ((ret = fat32_rootdir32_open(disk, dir->fs_dir)) < 0)
    {
        goto out;
    }

    strncpy(open_path, path, KERNEL_DISK_MAX_PATH);

    /* Pfade muessen mit einem Slash anfangen */
    if (open_path[0] != '/')
    {
        ret = -FAT_EINVAL;
        goto out;
    }

    /* Jetzt die Unterverzeichnisse parsen */
    p = strtok(&open_path[1], "/");
    while (p != NULL)
    {
        ret = fat32_subdir_open(disk, dir, p, &subdir);
        fat32_dir_close(disk, dir);

        if (ret < 0)
        {
            goto out;
        }
        *(struct fat32_dir *)(dir->fs_dir) = subdir;
        p = strtok(NULL, "/");
    }

    ret = 0;
out:
    return ret;
}

static int fat32_dir_close(struct disk *disk, struct dir *dir)
{
    struct fat32_dir *fs_dir = dir->fs_dir;

    if (fs_dir != NULL)
    {
        if (fs_dir->entries != NULL)
        {
            free(fs_dir->entries);
            fs_dir->entries = NULL;
        }
    }

    return 0;
}

static int fat32_dir_read(struct disk *disk, struct dir *dir, struct dir_entry *entry)
{
    struct fat32_dir *fs_dir = dir->fs_dir;
    struct fat32_dir_entry *fs_dir_entry;
    static struct fat32_dir_entry fs_dir_entry_public;

    if (entry->fs_dir_entry == NULL)
    {
        entry->fs_dir_entry = &fs_dir_entry_public;
    }

    fs_dir_entry = entry->fs_dir_entry;

    /* Ungueltige Eintraege ueberspringen */
    while (fs_dir->i < fs_dir->num && !is_valid_entry(&fs_dir->entries[fs_dir->i]))
    {
        short_fn_to_string(&fs_dir->entries[fs_dir->i], fs_dir_entry->name);
        fs_dir->i++;
    }

    /* Fertig? */
    if (fs_dir->i >= fs_dir->num)
    {
        return -1;
    }

    dir_entry_from_disk(fs_dir_entry, &fs_dir->entries[fs_dir->i]);
    fs_dir_entry->dir_first_cluster = fs_dir->first_cluster;
    fs_dir_entry->index_in_dir      = fs_dir->i;

    fs_dir->i++;

    entry->disk = dir->disk;
    entry->attribute = fs_dir_entry->attrib;
    strcpy(entry->name, fs_dir_entry->name);

    return 0;
}

static int fat32_dir_create_entry(struct disk *disk, struct dir *dir, const char *name, enum DIR_ENTRY_ATTRIB attribute, struct dir_entry *dir_entry)
{
    int i;
    int ret;
    struct fat32_dir *fs_dir = dir->fs_dir;
    struct fat32_dir_entry *fs_dir_entry = dir_entry->fs_dir_entry;

    /* Dateistruktur faken, deren Verzeichniseintrag "geaendert" werden soll */
    struct fat32_file file =
    {
        .fs = fs_dir->fs,
        .size = 0,
        .dir_entry =
        {
            .dir_first_cluster = fs_dir->first_cluster,
            .index_in_dir = 0,
            .first_cluster = 0,
            .attrib = attribute,
        },
    };

    /* Freien Index im Verzeichnis suchen */
    for (i = 0; i < fs_dir->num; ++i)
    {
        if (is_free_entry(&fs_dir->entries[i]))
        {
            goto found;
        }
    }

    /* Ausser dem Wurzelverzeichnis koennen wir alles vergroessern */
    if (fs_dir->first_cluster != 0)
    {
        i = fs_dir->num;
        goto found;
    }

    return -FAT_ENOENT;

found:
    strncpy(file.dir_entry.name, name, sizeof(file.dir_entry.name));
    file.dir_entry.name[sizeof(file.dir_entry.name) - 1] = '\0';
    file.dir_entry.index_in_dir = i;

    /* Ins Verzeichnis eintragen */
    if ((ret = fat32_dir_update_entry(disk, &file)) < 0)
    {
        return ret;
    }

    if (fs_dir_entry)
    {
        *fs_dir_entry = file.dir_entry;
    }

    return 0;
}

static int fat32_dir_create(struct disk *disk, struct dir *dir, const char *name, struct dir_entry *dir_entry)
{
    struct file dir_file;
    struct dir_entry dir_entry_tmp;
    struct fat32_fs *fs = disk->fs;
    struct fat32_file fs_dir_file;
    struct fat32_dir_entry fs_dir_entry;
    int ret;
    uint32_t cluster_size = fs->bpb.cluster_sectors * 512;
    uint8_t buf[cluster_size];
    struct fat32_disk_dir_entry *disk_dentry;

    dir_file.fs_file = &fs_dir_file;
    dir_entry_tmp.fs_dir_entry = &fs_dir_entry;

    /* Verzeichnis anlegen */
    if ((ret = fat32_dir_create_entry(disk, dir, name, FAT_ATTRIB_DIR, &dir_entry_tmp)) < 0)
    {
        return ret;
    }

    /* Und einen genullten Cluster reservieren */
    if ((ret = fat32_file_open_by_dir_entry(disk, &fs_dir_entry, &fs_dir_file)) < 0)
    {
        return ret;
    }

    disk_dentry = (struct fat32_disk_dir_entry*)buf;
    memset(buf, 0, cluster_size);
    disk_dentry[0] = (struct fat32_disk_dir_entry)
    {
        .name           = ".       ",
        .ext            = "   ",
        .attrib         = FAT_ATTRIB_DIR,
        .first_cluster  = fs_dir_entry.first_cluster,
        .size           = 0,
    };
    disk_dentry[1] = (struct fat32_disk_dir_entry)
    {
        .name           = "..      ",
        .ext            = "   ",
        .attrib         = FAT_ATTRIB_DIR,
        .first_cluster  = ((struct fat32_dir *)dir)->first_cluster,
        .size           = 0,
    };

    ret = fat32_file_write(disk, &dir_file, buf, 0, cluster_size);
    fat32_file_close(disk, &dir_file);

    if (dir_entry)
    {
        *(struct fat32_dir_entry *)dir_entry->fs_dir_entry = fs_dir_entry;
    }

    return ret;
}

int fat32_request(struct disk *disk, enum FS_REQUEST_TYPE type, void *params, void *ret)
{
    switch (type)
    {
    case FS_FILE_SIZE:
        if (params != NULL && ret != NULL)
        {
            struct fat32_file *fs_file = ((struct file *)(params))->fs_file;
            *(size_t *)ret = fs_file->size;
        }
    break;
    case FS_DIR_ENTRY_SIZE:
        if (params != NULL && ret != NULL)
        {
            struct fat32_dir_entry *fs_dir_entry = ((struct dir_entry *)(params))->fs_dir_entry;
            *(size_t *)ret = fs_dir_entry->size;
        }
    break;
    default: break;
    }

    return -1;
}

int fat32_check(struct disk *disk)
{
    uint32_t total_sectors;
    uint32_t rootdir_sectors;
    uint32_t data_sectors;
    uint32_t fat_entry_size = 4;
    uint32_t entries_count = 0;
    uint32_t entries_end;
    uint32_t free_entries_count = 0;
    uint32_t fat_entry;
    uint16_t sector_cur;
    uint16_t sector_off;
    struct fat32_fs fs;
    struct fat32_fs_info fs_info;

    if (!disk->device_read(disk, 0, &fs.bpb, 1))
    {
        return -1;
    }

    if (fs.bpb.magic != 0xaa55 || fs.bpb.extended.boot_signature != 0x29)
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

    data_sectors = total_sectors - fs.bpb.reserved_sectors - (fs.bpb.num_fats * fs.bpb.fat_sectors) - rootdir_sectors;
    data_sectors -= (fs.bpb.num_fats * fs.bpb.extended.fat_sectors);

    fs.total_sectors = total_sectors;
    fs.first_data_cluster = total_sectors - data_sectors;

    if ((disk->fs = malloc(sizeof(struct fat32_fs))) == NULL)
    {
        return -1;
    }

    memcpy(disk->fs, &fs, sizeof(struct fat32_fs));

    disk->fs_file_open = &fat32_file_open;
    disk->fs_file_close = &fat32_file_close;
    disk->fs_file_read = &fat32_file_read;
    disk->fs_file_write = &fat32_file_write;
    disk->fs_dir_open = &fat32_dir_open;
    disk->fs_dir_close = &fat32_dir_close;
    disk->fs_dir_read = &fat32_dir_read;
    disk->fs_dir_create_entry = &fat32_dir_create_entry;
    disk->fs_dir_create = &fat32_dir_create;
    disk->fs_request = &fat32_request;
    disk->fs_type = "fat32";

    entries_end = fs.bpb.extended.fat_sectors * fs.bpb.sector_size / fat_entry_size;
    sector_cur = fs.bpb.reserved_sectors * fs.bpb.sector_size / fs.bpb.sector_size;
    sector_off = fs.bpb.reserved_sectors * fs.bpb.sector_size % fs.bpb.sector_size;

    disk->device_read(disk, sector_cur, fs_info.buffer, 1);

    while (entries_count < entries_end)
    {
        memcpy(&fat_entry, &fs_info.buffer[sector_off], fat_entry_size);

        if (fat_entry == 0)
        {
            ++free_entries_count;
        }

        sector_off += fat_entry_size;
        ++entries_count;

        if (sector_off >= fs.bpb.sector_size)
        {
            ++sector_cur;
            sector_off = 0;
            disk->device_read(disk, sector_cur, fs_info.buffer, 1);
        }
    }

    disk->free = free_entries_count * fs.bpb.cluster_sectors * fs.bpb.sector_size;
    disk->used = disk->total - disk->free;

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
    strcpy(&bpb.os_name[0], "TEST OS ");
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
    bpb.extended.boot_record_sectors = 6;
    memset(&bpb.extended.reserved[0], 0, 12);
    bpb.extended.drive_number = 0x80;
    bpb.extended.bootstrap_reserved = 0;
    bpb.extended.boot_signature = 0x29;
    bpb.extended.volume_serial_number = (uint32_t)get_timestamp();
    strcpy(&bpb.extended.volume_label[0], "NO NAME    ");
    strcpy(&bpb.extended.fs_type[0], "FAT32   ");
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
