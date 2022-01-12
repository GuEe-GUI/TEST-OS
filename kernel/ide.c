#include <device.h>
#include <disk.h>
#include <io.h>
#include <kernel.h>

/* ATA */
#define ATA_DRIVE_MASTER        0xA0
#define ATA_DRIVE_SLAVE         0xB0

#define ATA_BUS_PRIMARY         0x1f0
#define ATA_BUS_SECONDARY       0x170

#define ATA_CTL_PRI_PRIMARY     0x3f6
#define ATA_CTL_PRI_SECONDARY   0x376

#define ATA_REG_DATA            0x00
#define ATA_REG_ERROR_FEATURES  0x01
#define ATA_REG_SECTOR_CNT      0x02
#define ATA_REG_SECTOR_LOW      0x03
#define ATA_REG_SECTOR_MID      0x04
#define ATA_REG_SECTOR_HIGH     0x05
#define ATA_REG_DEVICE_SELECT   0x06
#define ATA_REG_COMMAND         0x07
#define ATA_REG_STATUS          ATA_REG_COMMAND

#define ATA_PIO_FLAG_READ       0x00
#define ATA_PIO_FLAG_WRITE      0x01

#define ATA_CMD_READ_PIO        0x20
#define ATA_CMD_READ_PIO_EXT    0x24
#define ATA_CMD_READ_MULTIPLE   0xc4
#define ATA_CMD_READ_DMA        0xC8
#define ATA_CMD_READ_DMA_EXT    0x25
#define ATA_CMD_WRITE_PIO       0x30
#define ATA_CMD_WRITE_PIO_EXT   0x34
#define ATA_CMD_WRITE_MULTIPLE  0xc5
#define ATA_CMD_WRITE_DMA       0xCA
#define ATA_CMD_WRITE_DMA_EXT   0x35
#define ATA_CMD_CACHE_FLUSH     0xE7
#define ATA_CMD_CACHE_FLUSH_EXT 0xEA
#define ATA_CMD_PACKET          0xA0
#define ATA_CMD_IDENTIFY_PACKET 0xA1
#define ATA_CMD_IDENTIFY        0xEC

#define ATA_SR_BSY              0x80    /* Busy */
#define ATA_SR_DRDY             0x40    /* Drive ready */
#define ATA_SR_DF               0x20    /* Drive write fault */
#define ATA_SR_DSC              0x10    /* Drive seek complete */
#define ATA_SR_DRQ              0x08    /* Data request ready */
#define ATA_SR_CORR             0x04    /* Corrected data */
#define ATA_SR_IDX              0x02    /* Index */
#define ATA_SR_ERR              0x01    /* Error */

#define IDE_INFO_BLOCK_SIZE     512

#define IDE_NAME(drive, bus)   \
    (drive == ATA_DRIVE_MASTER ? "master" : "slave"), \
    (bus == ATA_BUS_PRIMARY ? "primary" : "secondary")

struct ide
{
    uint8_t drive;
    uint16_t bus;
    uint16_t ctrl;
};

static struct ide ide_devices[4] =
{
    { .drive = ATA_DRIVE_MASTER, .bus = ATA_BUS_PRIMARY,   .ctrl = ATA_CTL_PRI_PRIMARY   },
    { .drive = ATA_DRIVE_SLAVE,  .bus = ATA_BUS_PRIMARY,   .ctrl = ATA_CTL_PRI_PRIMARY   },
    { .drive = ATA_DRIVE_MASTER, .bus = ATA_BUS_SECONDARY, .ctrl = ATA_CTL_PRI_SECONDARY },
    { .drive = ATA_DRIVE_SLAVE,  .bus = ATA_BUS_SECONDARY, .ctrl = ATA_CTL_PRI_SECONDARY },
};

static int ata_rw_pio(struct ide *ide_ata_device, int LBA, uint16_t *buffer, int sector_count, int pio_flag)
{
    int status, cmd, i, j;
    uint8_t drive = ide_ata_device->drive;
    uint16_t bus = ide_ata_device->bus;
    uint16_t ctrl = ide_ata_device->ctrl;

    /* 一次最多读取256个扇区 */
    if (sector_count > IDE_INFO_BLOCK_SIZE / 2)
    {
        goto fail;
    }
    sector_count = (uint8_t)sector_count;

    /* 一直读取状态，等待清除忙等状态 */
    while (io_in8(bus + ATA_REG_STATUS) & ATA_SR_BSY) {}

    io_out8(bus + ATA_REG_DEVICE_SELECT, (drive == ATA_DRIVE_MASTER ? 0xE0 : 0xF0) | ((LBA >> 24) & 0x0F));
    io_out8(bus + ATA_REG_SECTOR_CNT, sector_count);
    io_out8(bus + ATA_REG_SECTOR_LOW, (uint8_t)LBA);
    io_out8(bus + ATA_REG_SECTOR_MID, (uint8_t)(LBA >> 8));
    io_out8(bus + ATA_REG_SECTOR_HIGH, (uint8_t)(LBA >> 16));

    if (pio_flag == ATA_PIO_FLAG_WRITE)
    {
        cmd = (sector_count == 1) ? ATA_CMD_WRITE_PIO : ATA_CMD_WRITE_MULTIPLE;
    }
    else
    {
        cmd = (sector_count == 1) ? ATA_CMD_READ_PIO : ATA_CMD_READ_MULTIPLE;
    }
    io_out8(bus + ATA_REG_COMMAND, cmd);

    if (sector_count == 0)
    {
        sector_count = IDE_INFO_BLOCK_SIZE / 2;
    }

    for (j = 0; j < sector_count; j++)
    {
        for (i = 0; i < 4; ++i)
        {
            status = io_in8(ctrl);
        }

        while ((((status = io_in8(bus + ATA_REG_STATUS)) & ATA_SR_BSY) || !(status & ATA_SR_DRQ)) && !(status & ATA_SR_ERR) && !(status & ATA_SR_DF)) {}

        if ((status & ATA_SR_ERR) || (status & ATA_SR_DF))
        {
            printk("ata %s %d sectors from (%s:%s) fail\n", pio_flag == ATA_PIO_FLAG_WRITE ? "write" : "read", sector_count, IDE_NAME(drive, bus));
            goto fail;
        }

        for (i = 0; i < IDE_INFO_BLOCK_SIZE / 2; ++i, ++buffer)
        {
            /* 读写数据 */
            if (pio_flag == ATA_PIO_FLAG_WRITE)
            {
                io_out16(bus + ATA_REG_DATA, *buffer);
            }
            else
            {
                *buffer = io_in16(bus + ATA_REG_DATA);
            }
        }
        /* 写数据同步 */
        if (pio_flag == ATA_PIO_FLAG_WRITE)
        {
            io_out8(bus + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
        }
    }

    return 0;

fail:
    return -1;
}

static size_t ata_read(struct disk *disk, off_t sector_number, void *buffer, int sector_count)
{
    int status = ata_rw_pio((struct ide *)disk->device, sector_number, buffer, sector_count, ATA_PIO_FLAG_READ);

    return (status < 0 ? 0 : sector_count * IDE_INFO_BLOCK_SIZE);
}

static size_t ata_write(struct disk *disk, off_t sector_number, const void *buffer, int sector_count)
{
    int status = ata_rw_pio((struct ide *)disk->device, sector_number, (void *)buffer, sector_count, ATA_PIO_FLAG_WRITE);

    return (status < 0 ? 0 : sector_count * IDE_INFO_BLOCK_SIZE);
}

static int ide_identify(struct ide *ide_device)
{
    char *type;
    int status, i;
    uint8_t drive = ide_device->drive;
    uint16_t bus = ide_device->bus;
    uint16_t identify_data[IDE_INFO_BLOCK_SIZE / 2];
    struct disk *disk;

    io_out8(bus + ATA_REG_DEVICE_SELECT, drive);        /* 选择驱动器 */
    io_out8(bus + ATA_REG_SECTOR_CNT, 0);
    io_out8(bus + ATA_REG_SECTOR_LOW, 0);
    io_out8(bus + ATA_REG_SECTOR_MID, 0);
    io_out8(bus + ATA_REG_SECTOR_HIGH, 0);
    io_out8(bus + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);   /* 确认驱动器 */

    if ((status = io_in8(bus + ATA_REG_STATUS)) == 0)
    {
        LOG("ide(%s:%s) does not exist", IDE_NAME(drive, bus));

        goto fail;
    }
    else
    {
        uint8_t cl = io_in8(bus + ATA_REG_SECTOR_MID);
        uint8_t ch = io_in8(bus + ATA_REG_SECTOR_HIGH);

        do {
            if (cl == 0 && ch == 0)
            {
                type = "pata";
                break;
            }
            if (cl == 0x14 && ch == 0xEB)
            {
                type = "patapi";
            }
            if (cl == 0x69 && ch == 0x96)
            {
                type = "satapi";
            }
            if (cl == 0x3c && ch == 0xc3)
            {
                type = "sata";
            }

            LOG("ide(%s:%s) using %s is %s", IDE_NAME(drive, bus), type, "not supported yet");

            goto fail;
        } while (0);
    }

    /* 一直读取状态，等待驱动器清除忙等状态 */
    while (!((status = io_in8(bus + ATA_REG_STATUS)) & ATA_SR_DRQ) && !(status & ATA_SR_ERR)) {}

    if (status & ATA_SR_ERR)
    {
        LOG("ide(%s:%s) init fail", IDE_NAME(drive, bus));

        goto fail;
    }

    /* 读取驱动器信息 */
    for (i = 0; i < IDE_INFO_BLOCK_SIZE / 2; ++i)
    {
        identify_data[i] = io_in16(bus + ATA_REG_DATA);
    }

    if ((disk = disk_register("patapi ide")) != NULL)
    {
        uint32_t cylinders = identify_data[1];
        uint32_t heads = identify_data[3];
        uint32_t sector_count = identify_data[6];

        disk->device_read = &ata_read;
        disk->device_write = &ata_write;
        disk->device = ide_device;
        disk->total = (cylinders * heads * sector_count / 2048 + 1) * MB;

        LOG("ide(%s:%s) using %s is %s", IDE_NAME(drive, bus), type, "ready");
    }

    return 0;

fail:
    return -1;
}

void init_ide(void)
{
    ide_identify(&ide_devices[0]);
    ide_identify(&ide_devices[1]);
    ide_identify(&ide_devices[2]);
    ide_identify(&ide_devices[3]);
}
