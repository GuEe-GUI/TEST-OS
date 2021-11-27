#include <device.h>
#include <interrupt.h>
#include <io.h>
#include <kernel.h>
#include <malloc.h>
#include <thread.h>

/* ATA */
#define ATA_DRIVER_MASTER       0x00
#define ATA_DRIVER_SLAVE        0x01

#define ATA_PRIMARY_CMD_BASE    0x1f0
#define ATA_SECONDARY_CMD_BASE  0x170

#define ATA_PRIMARY_CTL_PRI     0x3f6
#define ATA_SECONDARY_CTL_PRI   0x376

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

#define ATA_READ_PIO_FLAG       0x00
#define ATA_WRITE_PIO_FLAG      0x01

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

#define IDE_NAME(driver, bus)   \
    (driver == ATA_DRIVER_MASTER ? "master" : "slave"), \
    (bus == ATA_PRIMARY_CMD_BASE ? "primary" : "secondary")

/* SATA */
#define SATA_LBAMID_PRI     0x1f4
#define SATA_LBAHI_PRI      0x1f5
#define SATA_INT_GET_RET    0x3c

struct ide
{
    uint8_t driver;
    uint16_t bus;
    uint16_t ctrl;
};

static struct ide ide_devices[4] =
{
    { .driver = ATA_DRIVER_MASTER, .bus = ATA_PRIMARY_CMD_BASE,   .ctrl = ATA_PRIMARY_CTL_PRI   },
    { .driver = ATA_DRIVER_SLAVE,  .bus = ATA_PRIMARY_CMD_BASE,   .ctrl = ATA_PRIMARY_CTL_PRI   },
    { .driver = ATA_DRIVER_MASTER, .bus = ATA_SECONDARY_CMD_BASE, .ctrl = ATA_SECONDARY_CTL_PRI },
    { .driver = ATA_DRIVER_SLAVE,  .bus = ATA_SECONDARY_CMD_BASE, .ctrl = ATA_SECONDARY_CTL_PRI },
};
static struct ide *ide_ata_device = NULL;

int ide_identify(struct ide *ide_device)
{
    int status, i;
    uint8_t driver = ide_device->driver;
    uint16_t bus = ide_device->bus;
    uint16_t identify_data[256] __attribute__((unused));

    status = io_in8(bus + ATA_REG_STATUS);

    io_out8(bus + ATA_REG_HDDEVSEL, 0xA0 | driver << 4);
    io_out8(bus + ATA_REG_SECCOUNT0, 0);
    io_out8(bus + ATA_REG_LBA0, 0);
    io_out8(bus + ATA_REG_LBA1, 0);
    io_out8(bus + ATA_REG_LBA2, 0);
    io_out8(bus + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

    if ((status = io_in8(bus + ATA_REG_STATUS)) == 0)
    {
        LOG("ide[%s:%s] does not exist\n", IDE_NAME(driver, bus));
        return -1;
    }

    while ((status = io_in8(bus + ATA_REG_STATUS)) & ATA_SR_BSY) {}

    if (io_in8(bus + ATA_REG_LBA1) == 0 && io_in8(bus + ATA_REG_LBA2) == 0)
    {
        while (!((status = io_in8(bus + ATA_REG_STATUS)) & ATA_SR_DRQ) && !(status & ATA_SR_ERR)) {}

        if (status & ATA_SR_ERR)
        {
            LOG("ide[%s:%s] using ata init fail\n", IDE_NAME(driver, bus));
            return -1;
        }

        for (i = 0; i < 256; i++)
        {
            identify_data[i] = io_in16(bus + ATA_REG_DATA);
        }

        ide_ata_device = ide_device;

        LOG("ide[%s:%s] using ata is ready\n", IDE_NAME(driver, bus));

        return 0;
    }
    else if (io_in8(SATA_LBAMID_PRI) == SATA_INT_GET_RET || io_in8(SATA_LBAHI_PRI) == SATA_INT_GET_RET)
    {
        LOG("ide[%s:%s] using %s is not supported yet\n", IDE_NAME(driver, bus), "sata");

        return -1;
    }
    else
    {
        LOG("ide[%s:%s] using %s is not supported yet\n", IDE_NAME(driver, bus), "atapi");

        return -1;
    }

    return 0;
}

static void ata_rw_pio(int LBA, void *buffer, int sector_count, int pio_flag)
{
    int status, i, j;
    uint8_t driver = ide_ata_device->driver;
    uint16_t bus = ide_ata_device->bus;
    uint16_t ctrl = ide_ata_device->ctrl;
    uint16_t *data;

    if (sector_count > 256)
    {
        return;
    }
    else if (sector_count == 256)
    {
        sector_count = 0; // 0 represnts 256 sectors
    }
    while (io_in8(bus + ATA_REG_STATUS) & ATA_SR_BSY) {} // Wait for ATA_SR_BSY to be zero.

    io_out8(bus + ATA_REG_HDDEVSEL, 0xE0 | (driver << 4) | ((LBA >> 24) & 0x0F));
    io_out8(bus + ATA_REG_SECCOUNT0, (unsigned char) sector_count);
    io_out8(bus + ATA_REG_LBA0, (unsigned char) LBA);
    io_out8(bus + ATA_REG_LBA1, (unsigned char) (LBA >> 8));
    io_out8(bus + ATA_REG_LBA2, (unsigned char) (LBA >> 16));
    int cmd = (sector_count == 1) ? ATA_CMD_READ_PIO :  ATA_CMD_READ_MULTIPLE;

    if (pio_flag == ATA_WRITE_PIO_FLAG)
    {
        cmd = (sector_count == 1) ? ATA_CMD_WRITE_PIO :  ATA_CMD_WRITE_MULTIPLE;
    }
    io_out8(bus + ATA_REG_COMMAND, cmd);

    if (sector_count == 0)
    {
        sector_count = 256;
    }
    data = buffer;

    for (j = 0; j < sector_count; j++)
    {
        for (i = 0; i < 4; i++)
        {
            status = io_in8(ctrl);
        }

        while ((((status = io_in8(bus + ATA_REG_STATUS)) & ATA_SR_BSY) || !(status & ATA_SR_DRQ)) && !(status & ATA_SR_ERR) && !(status & ATA_SR_DF)) {}

        if ((status & ATA_SR_ERR) || (status & ATA_SR_DF))
        {
            printk("ata %s %d sectors from [%s:%s] fail\n", pio_flag == ATA_WRITE_PIO_FLAG ? "write" : "read", sector_count, IDE_NAME(driver, bus));
            return;
        }

        for (i = 0; i < 256; i++, data++)
        {
            if (pio_flag == ATA_WRITE_PIO_FLAG)
            {
                io_out16(bus + ATA_REG_DATA, *data);
            }
            else
            {
                *data = io_in16(bus + ATA_REG_DATA);
            }
        }
        if (pio_flag == ATA_WRITE_PIO_FLAG)
        {
            io_out8(bus + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
        }
    }
}

void ata_read(int sector_number, void *buffer, int sector_count)
{
    ASSERT(ide_ata_device != NULL);
    ata_rw_pio(sector_number, buffer, sector_count, ATA_READ_PIO_FLAG);
}

void ata_write(int sector_number, void *buffer, int sector_count)
{
    ASSERT(ide_ata_device != NULL);
    ata_rw_pio(sector_number, buffer, sector_count, ATA_WRITE_PIO_FLAG);
}

void init_ide()
{
    ide_identify(&ide_devices[0]);
    ide_identify(&ide_devices[1]);
    ide_identify(&ide_devices[2]);
    ide_identify(&ide_devices[3]);
}
