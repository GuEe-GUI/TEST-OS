#include <fat32.h>
#include <kernel.h>

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;

struct BootSector_BPB
{
    BYTE  BS_jmpBoot[3];
    BYTE  BS_OEMName[8];
    WORD  BPB_BytsPerSec;
    BYTE  BPB_SecPerClus;
    WORD  BPB_RsvdSecCnt;
    BYTE  BPB_NumFATs;
    WORD  BPB_RootEntCnt;
    WORD  BPB_TotSec16;
    BYTE  BPB_Media;
    WORD  BPB_FATSz16;
    WORD  BPB_SecPerTrk;
    WORD  BPB_NumHeads;
    DWORD BPB_HiddSec;
    DWORD BPB_TotSec32;
/* FAT32 offset */
    DWORD BPB_FATSz32;
    WORD  BPB_ExtFlags;
    WORD  BPB_FSVer;
    DWORD BPB_RootClus;
    WORD  BPB_FSInfo;
    WORD  BPB_BkBootSec;
    BYTE  BPB_Reserved[12];
    BYTE  BS_DrvNum;
    BYTE  BS_Reserved1;
    BYTE  BS_BootSig;
    DWORD BS_VolID;
    BYTE  BS_VolLab[11];
#define BS_FILSYSTYPE "FAT32  "
    BYTE  BS_FilSysType[8];
} __attribute__ ((packed));

struct FSInfo_Structure
{
#define FSI_LEADSIG 0x41615252
    DWORD FSI_LeadSig;
    BYTE  FSI_Reserved1[480];
#define FSI_STRUCSIG 0x61417272
    DWORD FSI_StrucSig;
    DWORD FSI_Free_Count;
    DWORD FSI_Nxt_Free;
    BYTE  FSI_Reserved2[12];
#define FSI_TRAILSIG 0xAA550000
    DWORD FSI_TrailSig;
} __attribute__ ((packed));

struct Byte_Directory_Entry
{
    BYTE  DIR_Name[11];
#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20
#define ATTR_LONG_NAME  (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)
    BYTE  DIR_Attr;
    BYTE  DIR_NTRes;
    BYTE  DIR_CrtTimeTenth;
    WORD  DIR_CrtTime;
    WORD  DIR_CrtDate;
    WORD  DIR_LstAccDate;
    WORD  DIR_FstClusHI;
    WORD  DIR_WrtTime;
    WORD  DIR_WrtDate;
    WORD  DIR_FstClusLO;
    DWORD DIR_FileSize;
} __attribute__ ((packed));

struct Long_Directory_Entry
{
    BYTE LDIR_Ord;
    BYTE LDIR_Name1[10];
#define LDIR_ATTR ATTR_LONG_NAME
    BYTE LDIR_Attr;
    BYTE LDIR_Type;
    BYTE LDIR_Chksum;
    BYTE LDIR_Name2[12];
    WORD LDIR_FstClusLO;
    BYTE LDIR_Name3[4];
} __attribute__ ((packed));

struct DSKSZTOSECPERCLUS
{
    DWORD DiskSize;
    BYTE SecPerClusVal;
} __attribute__ ((packed));

/*
 * This is the table for FAT32 drives. NOTE that this table includes
 * entries for disk sizes smaller than 512 MB even though typically
 * only the entries for disks >= 512 MB in size are used.
 * The way this table is accessed is to look for the first entry
 * in the table for which the disk size is less than or equal
 * to the DiskSize field in that table entry. For this table to
 * work properly BPB_RsvdSecCnt must be 32, and BPB_NumFATs
 * must be 2. Any of these values being different may require the first
 * table entries DiskSize value to be changed otherwise the cluster count
 * may be to low for FAT32.
 */
struct DSKSZTOSECPERCLUS DskTableFAT32[] =
{
    { 66600, 0 },       /* disks up to 32.5 MB, the 0 value for SecPerClusVal trips an error */
    { 532480, 1 },      /* disks up to 260 MB, .5k cluster */
    { 16777216, 8 },    /* disks up to 8 GB, 4k cluster */
    { 33554432, 16 },   /* disks up to 16 GB, 8k cluster */
    { 67108864, 32 },   /* disks up to 32 GB, 16k cluster */
    { 0xFFFFFFFF, 64 }  /* disks greater than 32GB, 32k cluster */
};

int fat32_check(struct disk *disk)
{
    return 0;
}

int fat32_format(struct disk *disk)
{
    return 0;
}
