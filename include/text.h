#ifndef _TEXT_H_
#define _TEXT_H_

#include <types.h>

int base10_string(uint32_t number, unsigned char sign, char dst[]);
int base8_string(uint32_t number, unsigned char sign, char dst[]);
int base16_string(uint32_t number, unsigned char sign, char dst[]);
int base2_string(uint32_t number, unsigned char sign, char dst[]);

#endif
