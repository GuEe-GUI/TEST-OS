#ifndef _BITMAP_H_
#define _BITMAP_H_

#include <types.h>

struct bitmap
{
    uint8_t *map;
    uint32_t map_size;

    uint32_t bits_ptr;
    uint32_t bits_start;
    uint32_t bits_end;
};

void init_bitmap(uint32_t bytes_len);
void *bitmap_malloc(size_t size);
void bitmap_free(void *addr);

#endif
