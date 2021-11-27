#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <types.h>

void init_keyboard(void);
uint8_t get_key();

void init_ide();
void ata_read(int sector_number, void *buffer, int sector_count);
void ata_write(int sector_number, void *buffer, int sector_count);

#define disk_read(offset, buffer, count) ata_read(offset, buffer, count);
#define disk_write(offset, buffer, count) ata_write(offset, buffer, count);

#endif
