#ifndef _TEXT_H_
#define _TEXT_H_

#include <types.h>

char *to_dec_string(int decimal_number);
char *to_udec_string(uint32_t decimal_number);
char *to_oct_string(uint32_t decimal_number);
char *to_hex_string(uint32_t decimal_number, bool_t sign);
char *to_bin_string(uint32_t decimal_number);

#endif
