#include <device.h>
#include <disk.h>
#include <io.h>
#include <kernel.h>

/* ATA */
#define ATA_TYPE_MASTER         0x00
#define ATA_TYPE_SLAVE          0x01

#define ATA_DRIVE_MASTER        0xA0
#define ATA_DRIVE_SLAVE         0xB0

#define ATA_BUS_PRIMARY         0x1f0
#define ATA_BUS_SECONDARY       0x170

#define ATA_CTL_PRI_PRIMARY     0x3f6
#define ATA_CTL_PRI_SECONDARY   0x376

#define ATA_REG_DATA            0x00
#define ATA_REG_ERROR           0x01
#define ATA_REG_FEATURES        0x01
#define ATA_REG_SECCOUNT0       0x02
#define ATA_REG_LBA0            0x03
#define ATA_REG_LBA1            0x04
#define ATA_REG_LBA2            0x05
#define ATA_REG_HDDEVSEL        0x06
#define ATA_REG_COMMAND         0x07
#define ATA_REG_STATUS          0x07
#define ATA_REG_SECCOUNT1       0x08
#define ATA_REG_LBA3            0x09
#define ATA_REG_LBA4            0x0A
#define ATA_REG_LBA5            0x0B
#define ATA_REG_CONTROL         0x0C
#define ATA_REG_ALTSTATUS       0x0C
#define ATA_REG_DEVADDRESS      0x0D

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

/* SATA */
#define SATA_LBAMID_PRI         0x1f4
#define SATA_LBAHI_PRI          0x1f5
#define SATA_INT_GET_RET        0x3c

#define IDE_INFO_BLOCK_SIZE     512

#define IDE_NAME(type, bus)   \
    (type == ATA_TYPE_MASTER ? "master" : "slave"), \
    (bus == ATA_BUS_PRIMARY ? "primary" : "secondary")

struct ide
{
    uint8_t type;
    uint8_t drive;
    uint16_t bus;
    uint16_t ctrl;
};

static struct ide ide_devices[4] =
{
    { .type = ATA_TYPE_MASTER, .drive = ATA_DRIVE_MASTER, .bus = ATA_BUS_PRIMARY,   .ctrl = ATA_CTL_PRI_PRIMARY   },
    { .type = ATA_TYPE_SLAVE,  .drive = ATA_DRIVE_SLAVE,  .bus = ATA_BUS_PRIMARY,   .ctrl = ATA_CTL_PRI_PRIMARY   },
    { .type = ATA_TYPE_MASTER, .drive = ATA_DRIVE_MASTER, .bus = ATA_BUS_SECONDARY, .ctrl = ATA_CTL_PRI_SECONDARY },
    { .type = ATA_TYPE_SLAVE,  .drive = ATA_DRIVE_SLAVE,  .bus = ATA_BUS_SECONDARY, .ctrl = ATA_CTL_PRI_SECONDARY },
};

static void ata_rw_pio(struct ide *ide_ata_device, int LBA, uint16_t *buffer, int sector_count, int pio_flag)
{
    int status, i, j;
    uint8_t type = ide_ata_device->type;
    uint16_t bus = ide_ata_device->bus;
    uint16_t ctrl = ide_ata_device->ctrl;

    if (sector_count > 256)
    {
        return;
    }
    else if (sector_count == 256)
    {
        sector_count = 0;
    }
    /* 判断等待状态 */
    while (io_in8(bus + ATA_REG_STATUS) & ATA_SR_BSY) {}

    io_out8(bus + ATA_REG_HDDEVSEL, 0xE0 | (type << 4) | ((LBA >> 24) & 0x0F));
    io_out8(bus + ATA_REG_SECCOUNT0, (unsigned char) sector_count);
    io_out8(bus + ATA_REG_LBA0, (unsigned char) LBA);
    io_out8(bus + ATA_REG_LBA1, (unsigned char) (LBA >> 8));
    io_out8(bus + ATA_REG_LBA2, (unsigned char) (LBA >> 16));
    int cmd = (sector_count == 1) ? ATA_CMD_READ_PIO :  ATA_CMD_READ_MULTIPLE;

    if (pio_flag == ATA_PIO_FLAG_WRITE)
    {
        cmd = (sector_count == 1) ? ATA_CMD_WRITE_PIO :  ATA_CMD_WRITE_MULTIPLE;
    }
    io_out8(bus + ATA_REG_COMMAND, cmd);

    if (sector_count == 0)
    {
        sector_count = 256;
    }

    for (j = 0; j < sector_count; j++)
    {
        for (i = 0; i < 4; i++)
        {
            status = io_in8(ctrl);
        }

        while ((((status = io_in8(bus + ATA_REG_STATUS)) & ATA_SR_BSY) || !(status & ATA_SR_DRQ)) && !(status & ATA_SR_ERR) && !(status & ATA_SR_DF)) {}

        if ((status & ATA_SR_ERR) || (status & ATA_SR_DF))
        {
            printk("ata %s %d sectors from [%s:%s] fail\n", pio_flag == ATA_PIO_FLAG_WRITE ? "write" : "read", sector_count, IDE_NAME(type, bus));
            return;
        }

        for (i = 0; i < 256; ++i, ++buffer)
        {
            if (pio_flag == ATA_PIO_FLAG_WRITE)
            {
                io_out16(bus + ATA_REG_DATA, *buffer);
            }
            else
            {
                *buffer = io_in16(bus + ATA_REG_DATA);
            }
        }
        if (pio_flag == ATA_PIO_FLAG_WRITE)
        {
            io_out8(bus + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
        }
    }
}

static size_t ata_read(struct disk *disk, size_t sector_number, void *buffer, int sector_count)
{
    ata_rw_pio((struct ide *)disk->device, sector_number, buffer, sector_count, ATA_PIO_FLAG_READ);

    return sector_count * IDE_INFO_BLOCK_SIZE;
}

static size_t ata_write(struct disk *disk, size_t sector_number, void *buffer, int sector_count)
{
    ata_rw_pio((struct ide *)disk->device, sector_number, buffer, sector_count, ATA_PIO_FLAG_WRITE);

    return sector_count * IDE_INFO_BLOCK_SIZE;
}

int ide_identify(struct ide *ide_device)
{
    int status, i;
    uint8_t type = ide_device->type;
    uint16_t bus = ide_device->bus;
    uint16_t identify_data[256] __attribute__((unused));
    struct disk *disk;

    status = io_in8(bus + ATA_REG_STATUS);

    io_out8(bus + ATA_REG_HDDEVSEL, 0xA0 | type << 4);
    io_out8(bus + ATA_REG_SECCOUNT0, 0);
    io_out8(bus + ATA_REG_LBA0, 0);
    io_out8(bus + ATA_REG_LBA1, 0);
    io_out8(bus + ATA_REG_LBA2, 0);
    io_out8(bus + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    if ((status = io_in8(bus + ATA_REG_STATUS)) == 0)
    {
        LOG("ide[%s:%s] does not exist\n", IDE_NAME(type, bus));

        return -1;
    }

    while ((status = io_in8(bus + ATA_REG_STATUS)) & ATA_SR_BSY) {}

    if (io_in8(bus + ATA_REG_LBA1) == 0 && io_in8(bus + ATA_REG_LBA2) == 0)
    {
        while (!((status = io_in8(bus + ATA_REG_STATUS)) & ATA_SR_DRQ) && !(status & ATA_SR_ERR)) {}

        if (status & ATA_SR_ERR)
        {
            LOG("ide[%s:%s] using ata init fail\n", IDE_NAME(type, bus));

            return -1;
        }

        for (i = 0; i < 256; i++)
        {
            identify_data[i] = io_in16(bus + ATA_REG_DATA);
        }

        disk = disk_register("ata ide", &ata_read, &ata_write, ide_device);

        if (disk != NULL)
        {
            uint32_t cylinders = identify_data[1];
            uint32_t heads = identify_data[3];
            uint32_t sector_count = identify_data[6];

            disk->total = (cylinders * heads * sector_count / 2048 + 1) * MB;

            LOG("ide[%s:%s] using ata is ready\n", IDE_NAME(type, bus));

            return 0;
        }

        return -1;
    }
    else if (io_in8(SATA_LBAMID_PRI) == SATA_INT_GET_RET || io_in8(SATA_LBAHI_PRI) == SATA_INT_GET_RET)
    {
        LOG("ide[%s:%s] using %s is not supported yet\n", IDE_NAME(type, bus), "sata");

        return -1;
    }
    else
    {
        LOG("ide[%s:%s] using %s is not supported yet\n", IDE_NAME(type, bus), "atapi");

        return -1;
    }

    return 0;
}

void init_ide()
{
    ide_identify(&ide_devices[0]);
    ide_identify(&ide_devices[1]);
    ide_identify(&ide_devices[2]);
    ide_identify(&ide_devices[3]);
}
