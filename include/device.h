#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <types.h>

void init_keyboard(void);
uint8_t get_key();

void init_ide();
void ata_read_lba_sector(unsigned long lba, uint8_t count, uint16_t *half_buf);
void ata_write_lba_sector(unsigned long lba, uint8_t count, uint16_t *half_buf);

#define read_sector(lba, count, half_buf) ata_read_lba_sector(lba, count, half_buf)
#define write_sector(lba, count, half_buf) ata_write_lba_sector(lba, count, half_buf)

#endif
