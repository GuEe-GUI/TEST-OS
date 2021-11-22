#include <device.h>
#include <interrupt.h>
#include <io.h>
#include <kernel.h>
#include <malloc.h>
#include <thread.h>

/* ATA */
#define ATA_SR_BSY          0x80    /* busy */
#define ATA_SR_DRDY         0x40    /* drive ready */
#define ATA_SR_DF           0x20    /* drive write fault */
#define ATA_SR_DSC          0x10    /* drive seek complete */
#define ATA_SR_DRQ          0x08    /* data request ready */
#define ATA_SR_CORR         0x04    /* corrected data */
#define ATA_SR_IDX          0x02    /* index */
#define ATA_SR_ERR          0x01    /* error */

#define ATA_EOI             0x20

#define ATA_MASTER          0x00
#define ATA_SLAVE           0x01

#define ATA_PRIMARY         0x00
#define ATA_SECONDARY       0x01

#define ATA_PRIMARY_CMD     0x1f0
#define ATA_SECONDARY_CMD   0x170

#define ATA_DRIVE_MASTER    0xa0
#define ATA_DRIVE_SLAVE     0xb0

#define ATA_IDENT_MODEL     54
#define ATA_IDENT_NAME_MAX  40

#define ATA_BUS_PRIMARY_CTL_PRI     0x1f7
#define ATA_BUS_SECONDARY_CTL_PRI   0x177

#define ATA_CMD_IDENTIFY            0xec
#define ATA_CMD_READ_SECTORS        0x20
#define ATA_CMD_WRITE_SECTORS       0x30
#define ATA_CMD_READ_MULTIPLE       0xc4
#define ATA_CMD_WRITE_MULTIPLE      0xc5
#define ATA_CMD_SET_MULTIPLE_MODE   0xc6
#define ATA_CMD_SET_FEATURES        0xef

/* SATA */
#define SATA_LBAMID_PRI     0x1f4
#define SATA_LBAHI_PRI      0x1f5
#define SATA_INT_GET_RET    0x3c

static uint8_t ide_identify_data[512];
static uint16_t ide_bus_cmd[] =
{
    [ATA_PRIMARY] = ATA_PRIMARY_CMD,
    [ATA_SECONDARY] = ATA_SECONDARY_CMD,
};
static uint8_t ide_ata_cmd = 0, ide_ata_drive = 0;

void ata_read_lba_sector(unsigned long lba, uint8_t count, uint16_t *half_buf)
{
}

void ata_write_lba_sector(unsigned long lba, uint8_t count, uint16_t *half_buf)
{
}

void ide_identify(uint8_t bus, uint8_t drive)
{
    uint16_t cmd = ide_bus_cmd[bus];

    io_out8(cmd + 6, drive == ATA_SLAVE ? ATA_DRIVE_SLAVE : ATA_DRIVE_MASTER);
    io_out8(cmd + 2, 0);
    io_out8(cmd + 3, 0);
    io_out8(cmd + 4, 0);
    io_out8(cmd + 5, 0);
    io_out8(cmd + 7, ATA_CMD_IDENTIFY);

    if (io_in8(cmd + 7) == 0)
    {
        LOG("ide[%d:%d] device does not exist\n", bus, drive);

        return;
    }

    if (io_in8(cmd + 4) == 0 && io_in8(cmd + 5) == 0)
    {
        int i;
        char *name = (char *)malloc(ATA_IDENT_NAME_MAX);

        ide_ata_cmd = cmd;
        ide_ata_drive = drive;

        for (i = 0; i < 256; ++i)
        {
            *(uint16_t *)(ide_identify_data + i * 2) = inportw(cmd);
        }
        for (i = 0; i < ATA_IDENT_NAME_MAX; i += 2)
        {
            name[i] = ide_identify_data[ATA_IDENT_MODEL + i + 1];
            name[i + 1] = ide_identify_data[ATA_IDENT_MODEL + i];
        }
        for (i = ATA_IDENT_NAME_MAX - 1; name[i] == ' '; --i)
        {
        }
        name[i + 1] = '\0';

        LOG("ide[%d:%d] device using ata name = \"%s\"\n", bus, drive, name);
        free(name);
    }
    else if (io_in8(SATA_LBAMID_PRI) == SATA_INT_GET_RET || io_in8(SATA_LBAHI_PRI) == SATA_INT_GET_RET)
    {
        LOG("ide[%d:%d] device using %s is not supported yet\n", bus, drive, "sata");

        return;
    }
    else
    {
        LOG("ide[%d:%d] device using %s is not supported yet\n", bus, drive, "atapi");

        return;
    }
}

void init_ide()
{
    ide_identify(ATA_PRIMARY, ATA_MASTER);
    ide_identify(ATA_PRIMARY, ATA_SLAVE);
    ide_identify(ATA_SECONDARY, ATA_MASTER);
    ide_identify(ATA_SECONDARY, ATA_SLAVE);
}
